
import sys
import os
import json
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    filename='ai_smart_filter.log')
logger = logging.getLogger("AISmartFilter")

from typing import List, Set, Optional, Dict, Any

class AISmartFilter:
    def __init__(self, model_name: str = "all-MiniLM-L6-v2"):
        self.use_ai: bool = False
        self.model: Any = None
        self.util: Any = None
        self.torch: Any = None
        self.embeddings: Any = None
        self.examples: List[str] = [] # List of strings that we want to filter out
        self.example_set: Set[str] = set()
        
        try:
            from sentence_transformers import SentenceTransformer, util
            import torch
            self.model = SentenceTransformer(model_name)
            self.util = util
            self.torch = torch
            self.use_ai = True
            logger.info(f"Loaded SentenceTransformer model: {model_name}")
        except ImportError:
            logger.warning("sentence-transformers not found. Falling back to heuristic mode.")
            print("WARNING: sentence-transformers not installed. AI Smart Filter running in simple mode.")
            self.use_ai = False
        except Exception as e:
            logger.error(f"Failed to load model: {e}")
            self.use_ai = False

        self.threshold: float = 0.75
        self.prediction_cache: Dict[str, bool] = {}

    def _clear_cache(self):
        self.prediction_cache.clear()

    def add_example(self, text):
        """Adds a text example to the filter list (stuff to skip)."""
        if not text or text in self.example_set:
            return
            
        self.examples.append(text)
        self.example_set.add(text)
        self._clear_cache()
        if self.use_ai and self.model:
            # Re-compute embeddings (naive approach for now, optimize later if needed)
            self._update_embeddings()
        
        logger.info(f"Added example: {text}")

    def remove_example(self, text):
        if text in self.example_set:
            self.examples.remove(text)
            self.example_set.discard(text)
            self._clear_cache()
            if self.use_ai and self.model:
                self._update_embeddings()
            logger.info(f"Removed example: {text}")

    def _update_embeddings(self):
        if not self.use_ai or not self.examples or self.model is None:
            self.embeddings = None
            return
        try:
            self.embeddings = self.model.encode(
                self.examples,
                convert_to_tensor=True,
                normalize_embeddings=True,
                show_progress_bar=False,
            )
        except Exception as e:
            logger.error(f"Error updating embeddings: {e}")
            self.embeddings = None

    def _has_embeddings(self):
        return self.embeddings is not None and len(self.examples) > 0

    def predict(self, text):
        """
        Returns True if the text should be SKIPPED (matches an example), False otherwise.
        """
        if not text:
            return True

        if not self.examples:
            return False

        # Check Cache
        if text in self.prediction_cache:
            return self.prediction_cache[text]

        result = False
        if self.use_ai and self.model and self.torch and self._has_embeddings():
            try:
                # Encode the new text
                query_embedding = self.model.encode(
                    text,
                    convert_to_tensor=True,
                    normalize_embeddings=True,
                    show_progress_bar=False,
                )
                
                # Embeddings are normalized, so dot product is cosine similarity.
                best_score = self.torch.max(self.torch.matmul(query_embedding, self.embeddings.T)).item()
                
                if best_score >= self.threshold:
                    logger.info(f"AI Skip: '{text}' (Score: {best_score:.2f})")
                    result = True
            except Exception as e:
                logger.error(f"Prediction error: {e}")
                # Fallback to exact match check
                pass
        
        # Fallback or Non-AI mode: Exact or simple substring match
        # We can implement a smarter non-AI fuzzy match here if we want (e.g. Levinshtein)
        # For now, let's just stick to exact match which is implicitly handled by Logic in C++ usually,
        # but here we are extending it. Let's do a simple containment check?
        # Actually, if the user provides "Var_01", they might expect "Var_02" to be skipped.
        # Without AI, that's hard unless we use regex which C++ side handles.
        # So for non-AI mode in Python, we might just rely on exact match or very simple logic.
        
        if not result:
             result = text in self.example_set
        
        self.prediction_cache[text] = result
        return result

    def predict_batch(self, texts):
        """
        Batch prediction for list of texts. Returns List[bool].
        """
        if not texts:
            return []
        
        results = [False] * len(texts)
        indices_to_predict = []
        texts_to_predict = []
        unique_texts = []
        unique_lookup = {}
        
        # 1. Check Cache first
        for i, text in enumerate(texts):
            if text in self.prediction_cache:
                results[i] = self.prediction_cache[text]
            elif self.examples and text in self.example_set: # Fast exact check
                 results[i] = True
                 self.prediction_cache[text] = True
            else:
                indices_to_predict.append(i)
                texts_to_predict.append(text)
                if text not in unique_lookup:
                    unique_lookup[text] = len(unique_texts)
                    unique_texts.append(text)
        
        if not indices_to_predict:
            return results

        if not self.use_ai or not self.model or not self.torch or not self._has_embeddings():
            # Fallback for all remaining (already checked exact match above)
            for i, text in zip(indices_to_predict, texts_to_predict):
                 self.prediction_cache[text] = False
            return results

        try:
            # Encode each distinct uncached text once.
            query_embeddings = self.model.encode(
                unique_texts,
                convert_to_tensor=True,
                normalize_embeddings=True,
                show_progress_bar=False,
            )
            
            # Embeddings are normalized, so dot product is cosine similarity.
            similarity_scores = self.torch.matmul(query_embeddings, self.embeddings.T)
            best_scores, _ = self.torch.max(similarity_scores, dim=1)

            unique_results = {}
            for text, unique_idx in unique_lookup.items():
                score = best_scores[unique_idx].item()
                should_skip = (score >= self.threshold)
                if should_skip:
                    logger.info(f"AI Skip (Batch): '{text}' (Score: {score:.2f})")
                unique_results[text] = should_skip
                self.prediction_cache[text] = should_skip
            
            for i, text in enumerate(texts_to_predict):
                original_idx = indices_to_predict[i]
                results[original_idx] = unique_results[text]

        except Exception as e:
            logger.error(f"Batch prediction error: {e}")
            # Fallback: defaults to False (Keep)
            pass
            
        return results

    def set_threshold(self, value):
        self.threshold = value
        self._clear_cache()

    def save_state(self, path):
        try:
            with open(path, 'w', encoding='utf-8') as f:
                json.dump({"examples": self.examples, "threshold": self.threshold}, f, ensure_ascii=False, indent=2)
            logger.info(f"State saved to {path}")
        except Exception as e:
            logger.error(f"Failed to save state: {e}")

    def load_state(self, path):
        if not os.path.exists(path):
            return
        try:
            with open(path, 'r', encoding='utf-8') as f:
                data = json.load(f)
                self.examples = data.get("examples", [])
                self.example_set = set(self.examples)
                self.threshold = data.get("threshold", 0.75)
            
            if self.use_ai and self.model:
                self._update_embeddings()
            
            logger.info(f"State loaded from {path}")
        except Exception as e:
            logger.error(f"Failed to load state: {e}")

# Global instance for easier access from C++ if needed, 
# though we will likely instantiate the class directly from C++.
filter_instance = AISmartFilter()

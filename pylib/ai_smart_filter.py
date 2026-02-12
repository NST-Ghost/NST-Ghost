
import sys
import os
import json
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    filename='ai_smart_filter.log')
logger = logging.getLogger("AISmartFilter")

class AISmartFilter:
    def __init__(self, model_name="all-MiniLM-L6-v2"):
        self.use_ai = False
        self.model = None
        self.embeddings = []
        self.examples = [] # List of strings that we want to filter out
        
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

        self.threshold = 0.75
        self.prediction_cache = {}

    def _clear_cache(self):
        self.prediction_cache.clear()

    def add_example(self, text):
        """Adds a text example to the filter list (stuff to skip)."""
        if not text or text in self.examples:
            return
            
        self.examples.append(text)
        self._clear_cache()
        if self.use_ai and self.model:
            # Re-compute embeddings (naive approach for now, optimize later if needed)
            self._update_embeddings()
        
        logger.info(f"Added example: {text}")

    def remove_example(self, text):
        if text in self.examples:
            self.examples.remove(text)
            self._clear_cache()
            if self.use_ai and self.model:
                self._update_embeddings()
            logger.info(f"Removed example: {text}")

    def _update_embeddings(self):
        if not self.use_ai or not self.examples:
            self.embeddings = []
            return
        try:
            self.embeddings = self.model.encode(self.examples, convert_to_tensor=True)
        except Exception as e:
            logger.error(f"Error updating embeddings: {e}")

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
        if self.use_ai and self.model and len(self.embeddings) > 0:
            try:
                # Encode the new text
                query_embedding = self.model.encode(text, convert_to_tensor=True)
                
                # Compute cosine similarities
                cosine_scores = self.util.cos_sim(query_embedding, self.embeddings)[0]
                
                # Find the best match
                best_score = self.torch.max(cosine_scores).item()
                
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
             result = text in self.examples
        
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
        
        # 1. Check Cache first
        for i, text in enumerate(texts):
            if text in self.prediction_cache:
                results[i] = self.prediction_cache[text]
            elif self.examples and text in self.examples: # Fast exact check
                 results[i] = True
                 self.prediction_cache[text] = True
            else:
                indices_to_predict.append(i)
                texts_to_predict.append(text)
        
        if not indices_to_predict:
            return results

        if not self.use_ai or not self.model or not self.embeddings is not None and len(self.embeddings) == 0:
            # Fallback for all remaining (already checked exact match above)
            for i, text in zip(indices_to_predict, texts_to_predict):
                 self.prediction_cache[text] = False
            return results

        try:
            # Batch Encode
            query_embeddings = self.model.encode(texts_to_predict, convert_to_tensor=True)
            
            # Compute Cosine Similarity against all examples
            # query_embeddings: [N, D]
            # self.embeddings: [M, D]
            # Result: [N, M]
            cosine_scores = self.util.cos_sim(query_embeddings, self.embeddings)
            
            # Max over examples (axis 1)
            # best_scores: [N]
            best_scores, _ = self.torch.max(cosine_scores, dim=1)
            
            for i, score_tensor in enumerate(best_scores):
                score = score_tensor.item()
                should_skip = (score >= self.threshold)
                
                original_idx = indices_to_predict[i]
                if should_skip:
                    logger.info(f"AI Skip (Batch): '{texts_to_predict[i]}' (Score: {score:.2f})")
                
                results[original_idx] = should_skip
                self.prediction_cache[texts_to_predict[i]] = should_skip

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
                self.threshold = data.get("threshold", 0.75)
            
            if self.use_ai and self.model:
                self._update_embeddings()
            
            logger.info(f"State loaded from {path}")
        except Exception as e:
            logger.error(f"Failed to load state: {e}")

# Global instance for easier access from C++ if needed, 
# though we will likely instantiate the class directly from C++.
filter_instance = AISmartFilter()

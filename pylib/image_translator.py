import sys
import os
import json
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    filename='image_translator.log')
logger = logging.getLogger("ImageTranslator")

# DEV MODE: Check if debug mode is enabled via environment variable
DEV_MODE = os.environ.get('NST_DEV_MODE', '0') == '1'

# Setup debug logger for dev mode
debug_logger = None
if DEV_MODE:
    debug_log_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'image_translator_debug.log')
    debug_logger = logging.getLogger("ImageTranslatorDebug")
    debug_logger.setLevel(logging.DEBUG)
    debug_handler = logging.FileHandler(debug_log_path, mode='w', encoding='utf-8')
    debug_handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    debug_logger.addHandler(debug_handler)
    debug_logger.info("=" * 60)
    debug_logger.info("NST Image Translator Debug Log (PaddleX)")
    debug_logger.info("=" * 60)

def debug_log(message, level="info"):
    """Write to debug log file if DEV_MODE is enabled."""
    if debug_logger:
        if level == "debug":
            debug_logger.debug(message)
        elif level == "warning":
            debug_logger.warning(message)
        elif level == "error":
            debug_logger.error(message)
        else:
            debug_logger.info(message)

class ImageTranslator:
    def __init__(self):
        self.pipeline = None
        self.use_gpu = False
        self.available = False
        self.gpu_status = "Unknown"
        self.device_name = "CPU"
        
        try:
            # Add user site-packages directly (bypassing PYTHONHOME isolation)
            import glob
            home_dir = os.path.expanduser("~")
            
            current_py_version = f"python{sys.version_info.major}.{sys.version_info.minor}"
            
            # 1. Add ~/.local/lib/pythonX.X/site-packages
            local_site = os.path.join(home_dir, ".local", "lib")
            matching_site = os.path.join(local_site, current_py_version, "site-packages")
            if os.path.isdir(matching_site) and matching_site not in sys.path:
                sys.path.insert(0, matching_site)
                logger.info(f"Added user site-packages for {current_py_version}: {matching_site}")
            
            # 2. Check VIRTUAL_ENV environment variable
            venv_path = os.environ.get("VIRTUAL_ENV")
            if venv_path:
                for pattern in [
                    os.path.join(venv_path, "lib", "python*", "site-packages"),
                    os.path.join(venv_path, "lib64", "python*", "site-packages")
                ]:
                    for sp in glob.glob(pattern):
                        if os.path.isdir(sp) and sp not in sys.path:
                            sys.path.insert(0, sp)
                            logger.info(f"Added venv site-packages: {sp}")
            
            # 3. Log final sys.path for debugging
            logger.debug(f"Final sys.path: {sys.path}")
            
            debug_log(f"Python executable: {sys.executable}")
            debug_log(f"Python version: {sys.version}")
            debug_log("sys.path:")
            for i, p in enumerate(sys.path):
                debug_log(f"  [{i}] {p}")
            
            # ── Import PaddlePaddle ──
            try:
                import paddle
                debug_log(f"✓ paddle imported successfully (version: {paddle.__version__})")
                print(f"[DEBUG] ✓ paddle imported successfully (version: {paddle.__version__})", file=sys.stderr)
            except ImportError as e:
                debug_log(f"✗ paddle import FAILED: {e}", "error")
                print(f"[DEBUG] ✗ paddle import FAILED: {e}", file=sys.stderr)
                raise
            
            # ── Import PaddleX ──
            try:
                from paddlex import create_pipeline
                debug_log("✓ paddlex imported successfully")
                print(f"[DEBUG] ✓ paddlex imported successfully", file=sys.stderr)
            except ImportError as e:
                debug_log(f"✗ paddlex import FAILED: {e}", "error")
                print(f"[DEBUG] ✗ paddlex import FAILED: {e}", file=sys.stderr)
                raise
            
            self.paddle = paddle
            
            # ── GPU Detection ──
            if paddle.device.is_compiled_with_cuda() and paddle.device.cuda.device_count() > 0:
                try:
                    device_name = paddle.device.cuda.get_device_name(0)
                    
                    # Quick GPU test
                    paddle.set_device('gpu:0')
                    test_tensor = paddle.zeros([10, 10])
                    _ = paddle.matmul(test_tensor, test_tensor)
                    del test_tensor
                    
                    self.use_gpu = True
                    self.device_name = device_name
                    self.gpu_status = f"GPU Active: {device_name}"
                    logger.info(f"GPU test PASSED: {device_name}")
                    
                except Exception as gpu_error:
                    error_msg = str(gpu_error)
                    logger.warning(f"GPU test FAILED: {error_msg}")
                    self.gpu_status = f"GPU Error: {error_msg[:80]}. Using CPU."
                    self.use_gpu = False
                    self.device_name = "CPU (GPU fallback)"
                    paddle.set_device('cpu')
            else:
                self.gpu_status = "No GPU detected. Using CPU."
                self.device_name = "CPU"
                paddle.set_device('cpu')
                logger.info("No CUDA GPU available. Using CPU mode.")
            
            # ── Create PaddleX OCR Pipeline ──
            device = "gpu:0" if self.use_gpu else "cpu"
            debug_log(f"Creating PaddleX OCR pipeline on device: {device}")
            
            self.pipeline = create_pipeline(
                pipeline="OCR",
                device=device
            )
            
            self.available = True
            logger.info(f"ImageTranslator ready (PaddleX). Device: {self.device_name}, GPU: {self.use_gpu}")
            
        except ImportError as e:
            logger.warning(f"PaddlePaddle or PaddleX not found: {e}. Running in MOCK mode.")
            self.gpu_status = "Dependencies missing. Running in MOCK mode."
            self.available = False
        except Exception as e:
            logger.error(f"Failed to initialize PaddleX: {e}")
            self.gpu_status = f"Init Error: {str(e)[:80]}"
            self.available = False
    
    def get_device_info(self):
        """Return device information for UI display."""
        return {
            "available": self.available,
            "use_gpu": self.use_gpu,
            "device_name": self.device_name,
            "status": self.gpu_status
        }

    def is_available(self):
        return self.available
    
    def translate_image(self, image_path, source_lang='en', target_lang='th', 
                       use_preprocessing=True, confidence_threshold=0.3, 
                       use_gcv=False, gcv_key_path=""):
        """
        Detect text in an image using PaddleX OCR pipeline.
        
        Args:
            image_path: Path to image file
            source_lang: Source language code (informational, PaddleX auto-detects)
            target_lang: Target language code (not used for OCR)
            use_preprocessing: Unused, kept for API compatibility
            confidence_threshold: Minimum confidence score (0-1)
            use_gcv: Whether to use Google Cloud Vision instead
            gcv_key_path: Path to GCV key file
            
        Returns:
            JSON string containing list of detections with text, bbox, confidence
        """
        results = []
        
        if not os.path.exists(image_path):
            logger.error(f"Image not found: {image_path}")
            return json.dumps({"error": "Image file not found"})

        # Google Cloud Vision Path
        if use_gcv:
            if not gcv_key_path or not os.path.exists(gcv_key_path):
                logger.error("GCV requested but key file missing")
                return json.dumps({"error": "Google Cloud Vision Key file not found"})
            
            try:
                return self.translate_image_gcv(image_path, gcv_key_path)
            except Exception as e:
                logger.error(f"GCV Error: {e}")
                return json.dumps({"error": f"Google Cloud Vision Error: {str(e)}"})

        if self.available and self.pipeline is not None:
            try:
                logger.info(f"Running PaddleX OCR on: {image_path}")
                
                output = self.pipeline.predict(
                    input=image_path,
                    use_doc_orientation_classify=False,
                    use_doc_unwarping=False,
                    use_textline_orientation=False,
                    text_rec_score_thresh=confidence_threshold,
                )
                
                for res in output:
                    # res is a dict-like result with rec_texts, rec_scores, rec_polys
                    res_dict = res.get('res', res) if isinstance(res, dict) else res
                    
                    # Try to access the structured result
                    try:
                        rec_texts = res_dict.get('rec_texts', []) if isinstance(res_dict, dict) else getattr(res_dict, 'rec_texts', [])
                        rec_scores = res_dict.get('rec_scores', []) if isinstance(res_dict, dict) else getattr(res_dict, 'rec_scores', [])
                        rec_polys = res_dict.get('rec_polys', []) if isinstance(res_dict, dict) else getattr(res_dict, 'rec_polys', [])
                    except Exception:
                        # Fallback: try direct attribute access on result object
                        rec_texts = getattr(res, 'rec_texts', [])
                        rec_scores = getattr(res, 'rec_scores', [])
                        rec_polys = getattr(res, 'rec_polys', [])
                    
                    import numpy as np
                    
                    # Convert numpy arrays to lists if needed
                    if hasattr(rec_scores, 'tolist'):
                        rec_scores = rec_scores.tolist()
                    if hasattr(rec_texts, 'tolist'):
                        rec_texts = list(rec_texts)
                    
                    for i, text in enumerate(rec_texts):
                        text = str(text).strip()
                        if not text:
                            continue
                        
                        score = float(rec_scores[i]) if i < len(rec_scores) else 0.0
                        
                        if score < confidence_threshold:
                            logger.debug(f"Skipping low confidence: '{text}' ({score:.2f})")
                            continue
                        
                        # Convert polygon to bbox list [[x1,y1],[x2,y2],[x3,y3],[x4,y4]]
                        if i < len(rec_polys):
                            poly = rec_polys[i]
                            if hasattr(poly, 'tolist'):
                                poly = poly.tolist()
                            # rec_polys is 4-point polygon: [[x1,y1],[x2,y2],[x3,y3],[x4,y4]]
                            bbox_list = [[int(p[0]), int(p[1])] for p in poly]
                        else:
                            continue
                        
                        results.append({
                            "text": text,
                            "bbox": bbox_list,
                            "confidence": score
                        })
                
                logger.info(f"PaddleX OCR completed: {len(results)} detections above threshold {confidence_threshold}")
                    
            except Exception as e:
                logger.error(f"Error during PaddleX OCR: {e}")
                import traceback
                logger.error(traceback.format_exc())
                return json.dumps({"error": str(e)})
        else:
            # MOCK MODE
            logger.info("Mocking OCR result")
            results = [
                {
                    "text": "Hello World",
                    "bbox": [[50, 50], [200, 50], [200, 100], [50, 100]],
                    "confidence": 0.99
                },
                {
                    "text": "Sample Text",
                    "bbox": [[50, 150], [200, 150], [200, 200], [50, 200]],
                    "confidence": 0.95
                }
            ]
            
        return json.dumps(results, ensure_ascii=False)

    def translate_image_gcv(self, image_path, key_path):
        """
        Use Google Cloud Vision API for OCR.
        """
        try:
            from google.cloud import vision
            from google.oauth2 import service_account
            import io
        except ImportError:
            return json.dumps({"error": "google-cloud-vision library not installed. Run 'pip install google-cloud-vision'"})

        try:
            credentials = service_account.Credentials.from_service_account_file(key_path)
            client = vision.ImageAnnotatorClient(credentials=credentials)

            with io.open(image_path, 'rb') as image_file:
                content = image_file.read()

            image = vision.Image(content=content)
            response = client.text_detection(image=image)
            texts = response.text_annotations

            results = []
            if texts:
                for text in texts[1:]:
                    vertices = [[vertex.x, vertex.y] for vertex in text.bounding_poly.vertices]
                    results.append({
                        "text": text.description,
                        "bbox": vertices,
                        "confidence": 1.0
                    })

            if response.error.message:
                raise Exception(
                    '{}\nFor more info on error messages, check: '
                    'https://cloud.google.com/apis/design/errors'.format(
                        response.error.message))
                        
            return json.dumps(results, ensure_ascii=False)

        except Exception as e:
            logger.error(f"GCV Exception: {e}")
            raise e

    def inpaint_text_regions(self, image_path, detections_json):
        """
        Remove text from image using OpenCV inpainting and detect background colors.
        
        Returns:
            JSON string containing enriched detections with angle and text color.
        """
        try:
            import cv2
            import numpy as np
            import math
        except ImportError as e:
            return json.dumps({"error": f"OpenCV not available: {e}"})
        
        # Load image with OpenCV for processing
        img = cv2.imread(image_path)
        if img is None:
            return json.dumps({"error": "Failed to load image"})
            
        detections = json.loads(detections_json)
        mask = np.zeros(img.shape[:2], dtype=np.uint8)
        enriched_detections = []

        for detection in detections:
            bbox = detection.get("bbox", [])
            if len(bbox) >= 4:
                pts = np.array(bbox, dtype=np.int32)
                cv2.fillPoly(mask, [pts], 255)
                
                # --- Advanced Analysis ---
                x_min = max(0, min(p[0] for p in bbox))
                y_min = max(0, min(p[1] for p in bbox))
                x_max = min(img.shape[1], max(p[0] for p in bbox))
                y_max = min(img.shape[0], max(p[1] for p in bbox))
                
                roi = img[y_min:y_max, x_min:x_max]
                
                # Defaults
                bg_color = [255, 255, 255]
                text_color = [0, 0, 0]
                angle = 0.0
                is_dark = False

                if roi.size > 0:
                    # 1. Calculate Rotation Angle (from top edge)
                    p0 = bbox[0]
                    p1 = bbox[1]
                    dx = p1[0] - p0[0]
                    dy = p1[1] - p0[1]
                    angle = math.degrees(math.atan2(dy, dx))
                    
                    # 2. Extract Colors (Text vs BG) using Otsu binarization
                    try:
                        gray_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
                        _, bin_img = cv2.threshold(gray_roi, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
                        
                        white_pixels = cv2.countNonZero(bin_img)
                        total_pixels = bin_img.size
                        
                        if white_pixels > total_pixels / 2:
                            mask_text = cv2.bitwise_not(bin_img)
                            mask_bg = bin_img
                        else:
                            mask_text = bin_img
                            mask_bg = cv2.bitwise_not(bin_img)
                            
                        if cv2.countNonZero(mask_text) > 0:
                            text_mean = cv2.mean(roi, mask=mask_text)[:3]
                            text_color = [int(text_mean[2]), int(text_mean[1]), int(text_mean[0])]
                        
                        if cv2.countNonZero(mask_bg) > 0:
                            bg_mean = cv2.mean(roi, mask=mask_bg)[:3]
                            bg_color = [int(bg_mean[2]), int(bg_mean[1]), int(bg_mean[0])]
                    except Exception as e:
                        avg = cv2.mean(roi)[:3]
                        bg_color = [int(avg[2]), int(avg[1]), int(avg[0])]
                
                luminance = (0.299 * bg_color[0] + 0.587 * bg_color[1] + 0.114 * bg_color[2]) / 255.0
                is_dark = luminance < 0.5
                
                detection["bg_color"] = bg_color
                detection["text_color"] = text_color
                detection["angle"] = angle
                detection["is_dark"] = is_dark
                
                enriched_detections.append(detection)
        
        # --- OpenCV Inpainting ---
        # Dilate mask for better coverage
        kernel = np.ones((5,5), np.uint8)
        mask_dilated = cv2.dilate(mask, kernel, iterations=3)
        
        # Use INPAINT_TELEA for good quality text removal
        inpainted = cv2.inpaint(img, mask_dilated, 5, cv2.INPAINT_TELEA)
        
        import tempfile
        output_path = os.path.join(tempfile.gettempdir(), f"inpainted_{os.path.basename(image_path)}")
        cv2.imwrite(output_path, inpainted)
        
        logger.info(f"OpenCV Inpainting success: {output_path}")
        
        return json.dumps({
            "inpainted_path": output_path,
            "detections": enriched_detections
        })

# Global instance
translator_instance = ImageTranslator()
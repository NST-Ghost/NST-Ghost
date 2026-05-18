import sys
import os
import json
import logging

# Windows DLL Search Path Fix (Python 3.8+)
# Since Python 3.8, DLL loading is more restrictive for security.
# For bundled deployments (embedded Python), we need to explicitly add DLL directories.
if sys.platform == 'win32':
    try:
        # Strategy: Find the "Lib/site-packages" directory relative to this script
        # Structure in bundled app: NST_ROOT/scripts/image_translator.py
        # Python root is NST_ROOT/python
        # Site-packages is NST_ROOT/python/Lib/site-packages
        
        script_dir = os.path.dirname(os.path.abspath(__file__))
        nst_root = os.path.dirname(script_dir) # Go up one level from scripts/
        
        # Check standard embedded structure
        candidate_site_packages = [
            os.path.join(nst_root, 'python', 'Lib', 'site-packages'), # Bundled structure
            os.path.join(nst_root, 'Lib', 'site-packages'),           # Alternative structure
            os.path.join(os.path.dirname(sys.executable), 'Lib', 'site-packages'), # Standard Python
        ]
        
        found_site_packages = None
        for path in candidate_site_packages:
            if os.path.isdir(path):
                found_site_packages = path
                break
        
        if found_site_packages:
            # Add DLL search directories for critical AI packages
            # Torch 2.x+ usually puts DLLs in torch/lib
            torch_lib = os.path.join(found_site_packages, 'torch', 'lib')
            torch_bin = os.path.join(found_site_packages, 'torch', 'bin') # Sometimes here in newer builds
            numpy_libs = os.path.join(found_site_packages, 'numpy', '.libs')
            cv2_dir = os.path.join(found_site_packages, 'cv2')
            
            # Also add the main python dir for python3.dll etc
            python_dir = os.path.dirname(sys.executable)
            
            paths_to_add = [python_dir, torch_lib, torch_bin, numpy_libs, cv2_dir]
            
            for dll_dir in paths_to_add:
                if os.path.isdir(dll_dir):
                    try:
                        os.add_dll_directory(dll_dir)
                        # Also add to PATH as fallback for some legacy loads
                        os.environ['PATH'] = dll_dir + ';' + os.environ['PATH']
                    except Exception:
                        pass
                        
    except Exception as e:
        print(f"Warning: Failed to configure DLL search paths: {e}", file=sys.stderr)

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
    debug_logger.info("NST Image Translator Debug Log")
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
        self.reader = None
        self.reader_cache = {}  # Cache readers by language key
        self.use_gpu = False
        self.available = False
        self.gpu_status = "Unknown"
        self.device_name = "CPU"
        
        try:
            # Add user site-packages directly (bypassing PYTHONHOME isolation)
            # When PYTHONHOME is set by bundled Python, site.getusersitepackages() 
            # returns the bundled path instead of user's ~/.local/lib/pythonX.X/site-packages
            # IMPORTANT: Only add site-packages for the CURRENT Python version to avoid
            # loading incompatible packages from other Python versions.
            import glob
            home_dir = os.path.expanduser("~")
            
            # Get current Python version string (e.g., "python3.12")
            current_py_version = f"python{sys.version_info.major}.{sys.version_info.minor}"
            
            # 1. Add ~/.local/lib/pythonX.X/site-packages (user-installed packages)
            #    Only for the CURRENT Python version to prevent ABI mismatches
            local_site = os.path.join(home_dir, ".local", "lib")
            matching_site = os.path.join(local_site, current_py_version, "site-packages")
            if os.path.isdir(matching_site) and matching_site not in sys.path:
                sys.path.insert(0, matching_site)  # Priority over bundled
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
                            sys.path.insert(0, sp)  # Priority over bundled
                            logger.info(f"Added venv site-packages: {sp}")
            
            # 3. Log final sys.path for debugging
            logger.debug(f"Final sys.path: {sys.path}")
            
            # DEBUG: Print to stderr for immediate visibility and log to file
            import sys as _sys
            debug_log(f"Python executable: {_sys.executable}")
            debug_log(f"Python version: {_sys.version}")
            debug_log("sys.path:")
            print(f"[DEBUG] Python executable: {_sys.executable}", file=_sys.stderr)
            print(f"[DEBUG] Python version: {_sys.version}", file=_sys.stderr)
            print(f"[DEBUG] sys.path:", file=_sys.stderr)
            for i, p in enumerate(_sys.path):
                debug_log(f"  [{i}] {p}")
                print(f"  [{i}] {p}", file=_sys.stderr)
            
            # Try importing each dependency separately for better error messages
            try:
                import torch
                debug_log(f"✓ torch imported successfully (version: {torch.__version__})")
                print(f"[DEBUG] ✓ torch imported successfully (version: {torch.__version__})", file=_sys.stderr)
            except ImportError as e:
                debug_log(f"✗ torch import FAILED: {e}", "error")
                print(f"[DEBUG] ✗ torch import FAILED: {e}", file=_sys.stderr)
                
                # Enhanced diagnostics for WinError 126
                if "126" in str(e) and sys.platform == 'win32':
                    debug_log("=" * 60)
                    debug_log("DETAILED WinError 126 DIAGNOSIS")
                    debug_log("=" * 60)
                    print("\n" + "="*60, file=_sys.stderr)
                    print("DETAILED WinError 126 DIAGNOSIS", file=_sys.stderr)
                    print("="*60, file=_sys.stderr)
                    
                    import ctypes
                    
                    # Find torch lib directory
                    torch_lib_candidates = []
                    for path in sys.path:
                        torch_lib = os.path.join(path, 'torch', 'lib')
                        if os.path.isdir(torch_lib):
                            torch_lib_candidates.append(torch_lib)
                    
                    if torch_lib_candidates:
                        torch_lib = torch_lib_candidates[0]
                        debug_log(f"Torch lib directory: {torch_lib}")
                        print(f"Torch lib directory: {torch_lib}", file=_sys.stderr)
                        
                        # List all DLLs in torch/lib
                        debug_log("DLLs in torch/lib:")
                        print("\nDLLs in torch/lib:", file=_sys.stderr)
                        dlls_in_dir = [f for f in os.listdir(torch_lib) if f.endswith('.dll')]
                        for dll in sorted(dlls_in_dir):
                            debug_log(f"  - {dll}")
                            print(f"  - {dll}", file=_sys.stderr)
                        
                        # Try loading each critical DLL individually
                        critical_dlls = ['c10.dll', 'torch_cpu.dll', 'fbgemm.dll', 'libomp140.x86_64.dll', 'asmjit.dll']
                        debug_log("DLL Load Test (one by one):")
                        print("\nDLL Load Test (one by one):", file=_sys.stderr)
                        
                        for dll_name in critical_dlls:
                            dll_path = os.path.join(torch_lib, dll_name)
                            try:
                                if os.path.exists(dll_path):
                                    ctypes.CDLL(dll_path)
                                    debug_log(f"  ✓ {dll_name} - LOADABLE")
                                    print(f"  ✓ {dll_name} - LOADABLE", file=_sys.stderr)
                                else:
                                    debug_log(f"  ✗ {dll_name} - FILE NOT FOUND", "warning")
                                    print(f"  ✗ {dll_name} - FILE NOT FOUND", file=_sys.stderr)
                            except OSError as dll_err:
                                debug_log(f"  ✗ {dll_name} - LOAD FAILED: {dll_err}", "error")
                                print(f"  ✗ {dll_name} - LOAD FAILED: {dll_err}", file=_sys.stderr)
                                # This is likely the culprit!
                                if "126" in str(dll_err):
                                    debug_log(f"    ^ THIS DLL has missing dependencies!", "error")
                                    print(f"    ^ THIS DLL has missing dependencies!", file=_sys.stderr)
                    else:
                        debug_log("Could not find torch/lib directory!", "error")
                        print("Could not find torch/lib directory!", file=_sys.stderr)
                    
                    debug_log("=" * 60)
                    print("\n" + "="*60, file=_sys.stderr)
                
                raise
            
            try:
                import easyocr
                debug_log("✓ easyocr imported successfully")
                print(f"[DEBUG] ✓ easyocr imported successfully", file=_sys.stderr)
            except ImportError as e:
                debug_log(f"✗ easyocr import FAILED: {e}", "error")
                print(f"[DEBUG] ✗ easyocr import FAILED: {e}", file=_sys.stderr)
                raise
            
            self.easyocr = easyocr
            self.torch = torch
            
            # Step 1: Check if CUDA/ROCm is available
            if torch.cuda.is_available():
                device_name = torch.cuda.get_device_name(0)
                gpu_arch = "unknown"
                
                # Try to get GPU architecture for AMD
                try:
                    if hasattr(torch.version, 'hip'):
                        # ROCm - try to get actual architecture
                        props = torch.cuda.get_device_properties(0)
                        gpu_arch = getattr(props, 'gcnArchName', 'unknown')
                except:
                    pass
                
                # Step 2: Actually TEST the GPU with a small operation
                try:
                    logger.info(f"Testing GPU: {device_name} (arch: {gpu_arch})")
                    test_tensor = torch.zeros(10, 10, device='cuda')
                    result = torch.matmul(test_tensor, test_tensor)
                    del test_tensor, result
                    torch.cuda.empty_cache()
                    
                    # GPU works!
                    self.use_gpu = True
                    self.device_name = device_name
                    self.gpu_status = f"GPU Active: {device_name}"
                    logger.info(f"GPU test PASSED: {device_name}")
                    
                except Exception as gpu_error:
                    # GPU failed - likely unsupported architecture
                    error_msg = str(gpu_error)
                    logger.warning(f"GPU test FAILED: {error_msg}")
                    
                    if "rocBLAS" in error_msg or "TensileLibrary" in error_msg:
                        self.gpu_status = f"GPU Not Supported: {device_name} (architecture not supported by ROCm). Using CPU."
                        logger.warning(f"AMD GPU '{device_name}' with arch '{gpu_arch}' is not supported by ROCm. Falling back to CPU.")
                    elif "CUDA" in error_msg:
                        self.gpu_status = f"CUDA Error: {device_name}. Using CPU."
                        logger.warning(f"NVIDIA GPU error. Falling back to CPU.")
                    else:
                        self.gpu_status = f"GPU Error: {error_msg[:50]}. Using CPU."
                    
                    self.use_gpu = False
                    self.device_name = "CPU (GPU fallback)"
            else:
                # No GPU detected at all
                self.gpu_status = "No GPU detected. Using CPU."
                self.device_name = "CPU"
                logger.info("No CUDA/ROCm GPU available. Using CPU mode.")
            
            self.available = True
            logger.info(f"ImageTranslator ready. Device: {self.device_name}, GPU: {self.use_gpu}")
            
        except ImportError as e:
            logger.warning(f"EasyOCR or Torch not found: {e}. Running in MOCK mode.")
            self.gpu_status = "Dependencies missing. Running in MOCK mode."
            self.available = False
        except Exception as e:
            logger.error(f"Failed to initialize EasyOCR: {e}")
            self.gpu_status = f"Init Error: {str(e)[:50]}"
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
    
    def preprocess_image(self, image_path):
        """
        Pre-process image to improve OCR accuracy.
        Returns path to processed image.
        """
        try:
            import cv2
            import numpy as np
            from PIL import Image, ImageEnhance
            import tempfile
            
            # Read image
            img = cv2.imread(image_path)
            if img is None:
                return image_path
            
            # Convert to grayscale
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            
            # Apply adaptive thresholding for better text contrast
            # This works well for varying lighting conditions
            processed = cv2.adaptiveThreshold(
                gray, 255, 
                cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
                cv2.THRESH_BINARY, 
                11, 2
            )
            
            # Denoise
            processed = cv2.fastNlMeansDenoising(processed, None, 10, 7, 21)
            
            # Sharpen the image
            kernel = np.array([[-1,-1,-1],
                             [-1, 9,-1],
                             [-1,-1,-1]])
            processed = cv2.filter2D(processed, -1, kernel)
            
            # Slight dilation to make text thicker (helps with thin fonts)
            kernel = np.ones((2,2), np.uint8)
            processed = cv2.dilate(processed, kernel, iterations=1)
            
            # Save processed image
            temp_path = os.path.join(tempfile.gettempdir(), f"preprocessed_{os.path.basename(image_path)}")
            cv2.imwrite(temp_path, processed)
            
            logger.info(f"Image preprocessed: {temp_path}")
            return temp_path
            
        except Exception as e:
            logger.warning(f"Preprocessing failed: {e}. Using original image.")
            return image_path

    def translate_image(self, image_path, source_lang='en', target_lang='th', 
                       use_preprocessing=True, confidence_threshold=0.3, 
                       use_gcv=False, gcv_key_path=""):
        """
        Translates text in an image with enhanced accuracy.
        
        Args:
            image_path: Path to image file
            source_lang: Source language code
            target_lang: Target language code (not used in OCR, for future translation)
            use_preprocessing: Whether to preprocess image for better accuracy
            confidence_threshold: Minimum confidence score (0-1) to include detection
            
        Returns:
            JSON string containing list of detections with metadata
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

        if self.available:
            try:
                # Preprocess image if enabled
                processed_path = image_path
                if use_preprocessing:
                    processed_path = self.preprocess_image(image_path)
                
                # Strategy: For Japanese (and potentially other non-Latin languages), 
                # running separate passes for the specific language and English often yields 
                # better results than a single mixed-mode reader, especially for game UI 
                # where text types are distinct (e.g., Pixel English vs Hi-Res Japanese).
                
                pass_results = []
                
                # Define passes based on source_lang
                ocr_passes = []
                if source_lang == 'ja':
                    ocr_passes.append(['ja'])  # Dedicated Japanese pass
                    ocr_passes.append(['en'])  # Dedicated English pass
                else:
                    # Default behavior for other languages
                    langs = [source_lang]
                    if 'en' not in langs: 
                        langs.append('en')
                    ocr_passes.append(langs)

                # Execute OCR passes
                merged_detections = []
                
                for i, langs in enumerate(ocr_passes):
                    logger.info(f"Running OCR pass {i+1}/{len(ocr_passes)} with languages: {langs}")
                    
                    cache_key = tuple(sorted(langs))
                    if cache_key not in self.reader_cache:
                        logger.info(f"Creating new EasyOCR reader for: {langs}")
                        self.reader_cache[cache_key] = self.easyocr.Reader(
                            langs, 
                            gpu=self.use_gpu,
                            verbose=False
                        )
                    
                    reader = self.reader_cache[cache_key]
                    
                    # Run detections
                    detections = reader.readtext(
                        processed_path, 
                        detail=1,
                        paragraph=False,
                        min_size=10,
                        contrast_ths=0.1,
                        adjust_contrast=0.5,
                        width_ths=0.7,
                        link_threshold=0.4,
                        low_text=0.4,
                        text_threshold=0.7
                    )
                    
                    # Optional: Run on original if preprocessing was aggressive
                    if use_preprocessing and processed_path != image_path:
                        original_detections = reader.readtext(
                            image_path,
                            detail=1,
                            paragraph=False,
                            min_size=10,
                            contrast_ths=0.1,
                            adjust_contrast=0.5
                        )
                        detections = self._merge_detections(detections, original_detections)
                    
                    # Merge into main results
                    if not merged_detections:
                        merged_detections = detections
                    else:
                        merged_detections = self._merge_detections(merged_detections, detections)

                for bbox, text, prob in merged_detections:
                    # Filter by confidence threshold
                    if prob < confidence_threshold:
                        logger.debug(f"Skipping low confidence detection: '{text}' ({prob:.2f})")
                        continue
                    
                    # Clean up detected text
                    text = text.strip()
                    if not text:
                        continue
                    
                    # Convert bbox to list of integer coordinates
                    bbox_list = [[int(p[0]), int(p[1])] for p in bbox]
                    
                    results.append({
                        "text": text,
                        "bbox": bbox_list,
                        "confidence": float(prob)
                    })
                
                logger.info(f"OCR completed: {len(results)} detections above threshold {confidence_threshold}")
                    
            except Exception as e:
                logger.error(f"Error during OCR: {e}")
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
    
    def _merge_detections(self, det1, det2, iou_threshold=0.5):
        """
        Merge two sets of detections, removing duplicates based on IoU.
        Keeps detection with higher confidence.
        """
        import numpy as np
        
        def bbox_iou(box1, box2):
            """Calculate IoU between two bounding boxes."""
            # Convert to x_min, y_min, x_max, y_max format
            x1_min = min(p[0] for p in box1)
            y1_min = min(p[1] for p in box1)
            x1_max = max(p[0] for p in box1)
            y1_max = max(p[1] for p in box1)
            
            x2_min = min(p[0] for p in box2)
            y2_min = min(p[1] for p in box2)
            x2_max = max(p[0] for p in box2)
            y2_max = max(p[1] for p in box2)
            
            # Calculate intersection
            x_inter = max(0, min(x1_max, x2_max) - max(x1_min, x2_min))
            y_inter = max(0, min(y1_max, y2_max) - max(y1_min, y2_min))
            inter_area = x_inter * y_inter
            
            # Calculate union
            box1_area = (x1_max - x1_min) * (y1_max - y1_min)
            box2_area = (x2_max - x2_min) * (y2_max - y2_min)
            union_area = box1_area + box2_area - inter_area
            
            return inter_area / union_area if union_area > 0 else 0
        
        merged = list(det1)
        
        for bbox2, text2, prob2 in det2:
            is_duplicate = False
            
            for i, (bbox1, text1, prob1) in enumerate(merged):
                iou = bbox_iou(bbox1, bbox2)
                
                if iou > iou_threshold:
                    is_duplicate = True
                    # Keep the one with higher confidence
                    if prob2 > prob1:
                        merged[i] = (bbox2, text2, prob2)
                    break
            
            if not is_duplicate:
                merged.append((bbox2, text2, prob2))
        
        return merged

    def translate_image_gcv(self, image_path, key_path):
        """
        Use Google Cloud Vision API for OCR.
        """
        try:
            # pyrefly: ignore [missing-import]
            from google.cloud import vision
            # pyrefly: ignore [missing-import]
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
                # The first element is the entire text, specific words follow
                # We want specific words/blocks for partial replacement usually, 
                # but EasyOCR gives lines. GCV gives words or pages.
                # Let's iterate from index 1 to get individual text blocks/words
                # OR use the full_text_annotation if we want structure.
                # For consistency with current EasyOCR usage (lines), let's try to group?
                # Actually, simply taking texts[1:] is usually individual words.
                # To get lines like EasyOCR, we might need full_text_annotation.pages...
                
                # Simple approach first: Use the words (texts[1:])
                for text in texts[1:]:
                    vertices = [[vertex.x, vertex.y] for vertex in text.bounding_poly.vertices]
                    results.append({
                        "text": text.description,
                        "bbox": vertices,
                        "confidence": 1.0 # GCV doesn't provide word-level confidence easily in this view, usually high
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
        Remove text from image using AI (LaMa) inpainting and detect background colors.

        Returns:
            JSON string containing enriched detections with angle and text color.
        """
        SimpleLama = None
        Image = None
        try:
            import cv2
            import numpy as np
            import math
            from simple_lama_inpainting import SimpleLama as SL
            from PIL import Image as PIL_Image
            SimpleLama = SL
            Image = PIL_Image
        except ImportError as e:
            logger.error(f"Required library not available: {e}. Please run 'pip install simple-lama-inpainting'")
            # Fallback imports for color detection only if LaMa fails
            try:
                import cv2
                import numpy as np
                import math
            except:
                return json.dumps({"error": "OpenCV not available"})

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
                cv2.fillPoly(mask, [pts], (255,))

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
        
        # --- LaMa Inpainting ---
        try:
            if Image is not None and SimpleLama is not None:
                # Dilate mask for better coverage
                kernel = np.ones((5,5), np.uint8)
                mask_dilated = cv2.dilate(mask, kernel, iterations=3)
                
                # Convert to PIL for LaMa
                img_pil = Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
                mask_pil = Image.fromarray(mask_dilated)
                
                lama = SimpleLama()
                result_pil = lama(img_pil, mask_pil)
                
                # Save output
                import tempfile
                output_path = os.path.join(tempfile.gettempdir(), f"lama_inpainted_{os.path.basename(image_path)}")
                result_pil.save(output_path)
                
                logger.info(f"LaMa Inpainting success: {output_path}")
            else:
                logger.warning("LaMa or PIL not available. Falling back to OpenCV.")
                raise Exception("LaMa or PIL not available")
            
        except Exception as e:
            logger.error(f"LaMa Inpainting failed: {e}. Falling back to OpenCV.")
            # Fallback to OpenCV Telea
            kernel = np.ones((3,3), np.uint8)
            mask_dilated = cv2.dilate(mask, kernel, iterations=3)
            inpainted = cv2.inpaint(img, mask_dilated, 5, cv2.INPAINT_TELEA)
            
            import tempfile
            output_path = os.path.join(tempfile.gettempdir(), f"cv2_inpainted_{os.path.basename(image_path)}")
            cv2.imwrite(output_path, inpainted)
        
        return json.dumps({
            "inpainted_path": output_path,
            "detections": enriched_detections
        })

# Global instance
translator_instance = ImageTranslator()

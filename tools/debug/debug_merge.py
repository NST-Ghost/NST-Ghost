
import sys
import os
import logging
# Add the project root to sys.path
sys.path.append('/home/jop/work/NST/NST/scripts')
sys.path.append('/home/jop/work/NST/NST')

from image_translator import ImageTranslator

def test_merge_ocr(image_path):
    print(f"--- Testing MERGE Strategy for {image_path} ---")
    translator = ImageTranslator()
    
    # 1. Run English Pass
    print("Running English Pass...")
    # forcing source_lang='en' which creates Reader(['en'])
    # We cheat a bit by accessing internal helper if possible, but translate_image returns JSON string.
    res_en_json = translator.translate_image(image_path, source_lang='en', use_preprocessing=True)
    
    # 2. Run Japanese Pass
    print("Running Japanese Pass...")
    # forcing source_lang='ja' 
    # NOTE: current implementation of translate_image automatically adds 'en' if missing! 
    # Reader(['ja', 'en']). 
    # We saw this failed to get English text in previous run. 
    # But let's assume we treat it as the "JA focused" pass.
    res_ja_json = translator.translate_image(image_path, source_lang='ja', use_preprocessing=True)

    import json
    det_en = json.loads(res_en_json)
    det_ja = json.loads(res_ja_json)
    
    if "error" in det_en: det_en = []
    if "error" in det_ja: det_ja = []
    
    print(f"EN detections: {len(det_en)}")
    print(f"JA detections: {len(det_ja)}")

    # 3. Merge them manually using the translator's merge function
    # We need to convert list-of-dicts back to list-of-tuples for the internal function if it expects that?
    # Checking _merge_detections signature:
    # def _merge_detections(self, det1, det2, iou_threshold=0.5):
    # It expects standard EasyOCR format: (bbox, text, prob)
    # But our translate_image returns dicts: {'text':..., 'bbox':..., 'confidence':...}
    
    # Let's reimplement a simple merger here for the test script to verify the logic.
    
    def normalize_format(detections):
        # Convert from dict to (bbox, text, prob) just for processing if needed, 
        # or just work with the dicts directly if we write a custom merger.
        return detections

    def bbox_iou(box1, box2):
        # box is [[x,y], [x,y], [x,y], [x,y]]
        x1_min = min(p[0] for p in box1)
        y1_min = min(p[1] for p in box1)
        x1_max = max(p[0] for p in box1)
        y1_max = max(p[1] for p in box1)
        
        x2_min = min(p[0] for p in box2)
        y2_min = min(p[1] for p in box2)
        x2_max = max(p[0] for p in box2)
        y2_max = max(p[1] for p in box2)
        
        x_inter = max(0, min(x1_max, x2_max) - max(x1_min, x2_min))
        y_inter = max(0, min(y1_max, y2_max) - max(y1_min, y2_min))
        inter_area = x_inter * y_inter
        
        box1_area = (x1_max - x1_min) * (y1_max - y1_min)
        box2_area = (x2_max - x2_min) * (y2_max - y2_min)
        union_area = box1_area + box2_area - inter_area
        
        return inter_area / union_area if union_area > 0 else 0

    merged = list(det_en)
    
    print("\nMerging JA into EN...")
    for item_ja in det_ja:
        is_duplicate = False
        bbox_ja = item_ja['bbox']
        prob_ja = item_ja['confidence']
        
        for i, item_en in enumerate(merged):
            bbox_en = item_en['bbox']
            prob_en = item_en['confidence']
            
            iou = bbox_iou(bbox_ja, bbox_en)
            if iou > 0.3: # Threshold
                is_duplicate = True
                print(f"Overlap detected: '{item_ja['text']}' vs '{item_en['text']}' (IoU: {iou:.2f})")
                if prob_ja > prob_en:
                    print(f"  -> Replacing EN '{item_en['text']}' ({prob_en}) with JA '{item_ja['text']}' ({prob_ja})")
                    merged[i] = item_ja
                else:
                    print(f"  -> Keeping EN '{item_en['text']}' ({prob_en}) over JA ({prob_ja})")
                break
        
        if not is_duplicate:
            print(f"Adding new unique JA detection: '{item_ja['text']}'")
            merged.append(item_ja)
            
    print("\n--- Final Merged Results ---")
    print(json.dumps(merged, ensure_ascii=False, indent=2))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python debug_merge.py <image_path>")
        sys.exit(1)
        
    img_path = sys.argv[1]
    test_merge_ocr(img_path)

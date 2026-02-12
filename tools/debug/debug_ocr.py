
import sys
import os
import logging
# Add the project root to sys.path
sys.path.append('/home/jop/work/NST/NST/scripts')
sys.path.append('/home/jop/work/NST/NST')

from image_translator import ImageTranslator

def test_ocr(image_path, lang):
    print(f"--- Testing with lang='{lang}' ---")
    translator = ImageTranslator()
    # We won't use the full translate_image because we just want to see detections from internal logic 
    # but translate_image is the main entry point. 
    # We will use translate_image but print the raw json result.
    
    try:
        result_json = translator.translate_image(image_path, source_lang=lang)
        print(f"Result for {lang}:")
        print(result_json)
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python debug_ocr.py <image_path>")
        sys.exit(1)
        
    img_path = sys.argv[1]
    print(f"Testing image: {img_path}")
    
    # Test English only
    test_ocr(img_path, 'en')
    
    # Test Japanese (which includes English fallback in the code)
    test_ocr(img_path, 'ja')

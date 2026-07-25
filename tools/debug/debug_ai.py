import sys
import os
import platform
import subprocess
import glob

def print_header(title):
    print(f"\n{'='*60}")
    print(f" {title}")
    print(f"{'='*60}")

def check_dll_exists(name, search_paths):
    found = []
    for path in search_paths:
        if not os.path.isdir(path):
            continue
        full_path = os.path.join(path, name)
        if os.path.exists(full_path):
            found.append(full_path)
    return found

def main():
    print_header("NST AI Debugger Tool")
    
    print(f"Python Executable: {sys.executable}")
    print(f"Python Version: {sys.version}")
    print(f"Platform: {platform.platform()}")
    print(f"CWD: {os.getcwd()}")
    
    # 1. Check Python Environment
    print_header("Python Environment")
    print("search paths (sys.path):")
    for p in sys.path:
        print(f"  - {p}")
        
    # 2. Check DLL Search Mechanism
    print_header("DLL Search Configuration")
    if sys.platform == 'win32':
        if hasattr(os, 'add_dll_directory'):
            print("[OK] os.add_dll_directory is available (Python 3.8+)")
        else:
            print("! os.add_dll_directory is MISSING (Old Python?)")
            
        # Check PATH
        print("\nPATH Environment Variable (relevant entries):")
        for p in os.environ['PATH'].split(';'):
            if 'python' in p.lower() or 'torch' in p.lower() or 'cuda' in p.lower():
                print(f"  - {p}")
    
    # 3. Locate Site-Packages
    print_header("Checking Dependencies")
    
    # Heuristic to find site-packages
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir) # nst folder
    
    site_packages = None
    for p in sys.path:
        if 'site-packages' in p and os.path.isdir(p):
            site_packages = p
            break
            
    if site_packages:
        print(f"Found site-packages: {site_packages}")
        
        # Check Torch
        torch_dir = os.path.join(site_packages, 'torch')
        if os.path.exists(torch_dir):
            print(f"[OK] Torch directory found: {torch_dir}")
            
            # Check Torch Lib
            torch_lib = os.path.join(torch_dir, 'lib')
            if os.path.exists(torch_lib):
                print(f"[OK] Torch Lib found: {torch_lib}")
                
                # Check for critical DLLs
                critical_dlls = ['fbgemm.dll', 'torch_cpu.dll', 'c10.dll', 'libomp140.x86_64.dll']
                for dll in critical_dlls:
                    if os.path.exists(os.path.join(torch_lib, dll)):
                        print(f"  [OK] Found {dll}")
                    else:
                        print(f"  [FAIL] MISSING {dll}")
            else:
                print(f"[FAIL] Torch Lib NOT found at {torch_lib}")
        else:
            print(f"[FAIL] Torch NOT installed in {site_packages}")
    else:
        print("[FAIL] Could not locate site-packages directory")

    # 4. Attempt Import
    print_header("Attempting Imports")
    
    # 4.1 Try importing ctypes and loading VC Runtime
    try:
        import ctypes
        print("[OK] ctypes imported")
        
        # Try to find VCRUNTIME140.dll
        try:
            ctypes.CDLL("vcruntime140.dll")
            print("[OK] vcruntime140.dll loadable (VC++ Redist seems okay)")
        except Exception as e:
            print(f"[FAIL] Failed to load vcruntime140.dll: {e}")
            print("  -> Try installing Visual C++ Redistributable 2015-2022")
            
    except Exception as e:
        print(f"[FAIL] ctypes check failed: {e}")

    # 4.2 Try importing Torch
    print("\nImporting torch...")
    try:
        # Pre-import hook to mimic image_translator fix
        if sys.platform == 'win32' and site_packages:
            torch_lib = os.path.join(site_packages, 'torch', 'lib')
            if os.path.exists(torch_lib):
                try:
                    os.add_dll_directory(torch_lib)
                    print(f"  (Added {torch_lib} to DLL directory)")
                except:
                    pass
        
        import torch
        print(f"[OK] Torch imported successfully! Version: {torch.__version__}")
        print(f"   CUDA Available: {torch.cuda.is_available()}")
        
    except ImportError as e:
        print(f"[FAIL] ImportError loading torch: {e}")
    except OSError as e:
        print(f"[FAIL] OSError loading torch (likely DLL missing): {e}")
        if "126" in str(e):
            print("\nAnalysis: WinError 126 detected.")
            print("Suggestions:")
            print("1. Ensure 'libomp140.x86_64.dll' is in torch/lib.")
            print("2. Install Visual C++ Redistributable.")
            print("3. Check if your CPU supports basic AVX instructions.")
    except Exception as e:
        print(f"[FAIL] Unexpected error loading torch: {e}")

    print("\nPress Enter to exit...")
    input()

if __name__ == "__main__":
    main()

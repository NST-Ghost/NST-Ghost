import json
import sys
# pyrefly: ignore [missing-import]
import js2py  # Might not be installed, use simple check or fallback
# Better: Just print the scripts and I will eyeball them or use node to check specific ones.
# Actually, I can use subprocess to call node for syntax check.

import subprocess

def check_js_syntax(js_code, description):
    # Wrap in function to allow return statements
    wrapped_code = f"function check() {{ {js_code} }}"
    p = subprocess.Popen(['node', '-e', wrapped_code], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        # Check if error is SyntaxError
        error_msg = err.decode('utf-8')
        if "SyntaxError" in error_msg:
             print(f"Syntax Error in {description}:")
             print(f"Code: {js_code}")
             print(f"Error: {error_msg}")
             return True
    return False

try:
    with open('/home/jop/work/NST/NST/Unit-Test/RPGM-SyntaxError/CommonEvents-bug4.json', 'r', encoding='utf-8') as f:
        data = json.load(f)
except Exception as e:
    print(f"Failed to load JSON: {e}")
    sys.exit(1)

found_error = False

for i, evt in enumerate(data):
    if not evt: continue
    if 'list' not in evt: continue
    
    for j, cmd in enumerate(evt['list']):
        code = cmd.get('code')
        params = cmd.get('parameters', [])
        
        # Script
        if code in [355, 655]:
            if params and isinstance(params[0], str):
                if check_js_syntax(params[0], f"Event {i} Cmd {j} (Script)"):
                    found_error = True

        # Conditional Branch (Script)
        if code == 111 and params[0] == 12:
            if len(params) > 1 and isinstance(params[1], str):
                 # This is an expression, wrap in return
                 if check_js_syntax(f"return {params[1]};", f"Event {i} Cmd {j} (Cond Script)"):
                     found_error = True

        # Control Variables (Script)
        if code == 122 and params[3] == 4: # 4 = Script
            if len(params) > 4 and isinstance(params[4], str):
                 # This is an expression, wrap in return
                 if check_js_syntax(f"return {params[4]};", f"Event {i} Cmd {j} (Var Script)"):
                     found_error = True
                     
        # Plugin Command (356/357) - Check for weird chars
        if code in [356, 357]:
            if params and isinstance(params[0], str):
                 # Heuristic check
                 if "ZIndex" in params[0] and not params[0].startswith("PictureZIndex"):
                      print(f"Suspicious Plugin Command in Event {i} Cmd {j}: {params[0]}")

if not found_error:
    print("No JS Syntax Errors found.")

-- Unrpyc Installer Plugin
-- ติดตั้ง unrpyc สำหรับ decompile Ren'Py games

function on_install()
    nst_log("=== Installing unrpyc ===")
    
    local home = os.getenv("HOME")
    local install_dir = home .. "/.local/share/NST/unrpyc"
    
    nst_log("Install directory: " .. install_dir)
    
    -- สร้าง directory
    nst_log("Creating directory...")
    nst_install_package("mkdir -p " .. install_dir)
    
    -- ตรวจสอบว่ามี git ไหม
    nst_log("Checking git...")
    local has_git = nst_install_package("which git")
    if not has_git then
        nst_log("ERROR: git not found! Please install git first")
        return false
    end
    
    -- Clone unrpyc
    nst_log("Cloning unrpyc from GitHub...")
    local success = nst_install_package(
        "cd " .. install_dir .. " && git clone https://github.com/CensoredUsername/unrpyc.git . 2>&1"
    )
    
    if success then
        nst_log("✓ unrpyc installed successfully at: " .. install_dir)
        nst_log("You can now decompile .rpyc files!")
    else
        nst_log("✗ Failed to install unrpyc")
    end
    
    return success
end

function get_menu_items()
    return "Install/Check unrpyc"
end

function on_menu_click()
    nst_log("=== Checking unrpyc Installation ===")
    
    local home = os.getenv("HOME")
    local install_dir = home .. "/.local/share/NST/unrpyc"
    local unrpyc_path = install_dir .. "/unrpyc.py"
    
    -- ตรวจสอบว่าติดตั้งแล้วหรือยัง
    local check = nst_install_package("test -f " .. unrpyc_path)
    
    if check then
        nst_log("✓ unrpyc is installed at: " .. install_dir)
        nst_log("Usage: python3 " .. unrpyc_path .. " <file.rpyc>")
    else
        nst_log("✗ unrpyc not found")
        nst_log("Click 'Install Dependencies' to install it")
    end
end

-- Install unrpyc Plugin

function on_install()
    nst_log("Installing unrpyc...")
    
    local home = os.getenv("HOME")
    local install_dir = home .. "/.local/share/NST/unrpyc"
    
    -- Create directory
    nst_install_package("mkdir -p " .. install_dir)
    
    -- Clone unrpyc
    local success = nst_install_package(
        "cd " .. install_dir .. " && git clone https://github.com/CensoredUsername/unrpyc.git ."
    )
    
    if success then
        nst_log("unrpyc installed successfully!")
    else
        nst_log("Failed to install unrpyc")
    end
    
    return success
end

function get_menu_items()
    return "Decompile Ren'Py (.rpyc)"
end

function on_menu_click()
    nst_log("Decompile menu clicked!")
    -- TODO: Show file dialog and decompile
end

-- Custom Game Engine Support
-- เพิ่มการรองรับ engine ใหม่

function detect_engine(game_path)
    -- ตรวจสอบว่าเป็น engine ไหน
    nst_log("Detecting engine at: " .. game_path)
    
    -- Example: check for specific files
    if file_exists(game_path .. "/data.xp3") then
        return "kirikiri"
    end
    
    return "unknown"
end

function extract_text(file_path)
    nst_log("Extracting from: " .. file_path)
    
    -- Custom extraction logic
    local texts = {}
    -- ... extraction code ...
    
    return texts
end

function inject_translation(file_path, translations)
    nst_log("Injecting translations to: " .. file_path)
    
    -- Custom injection logic
    -- ... injection code ...
    
    return true
end

function file_exists(path)
    local f = io.open(path, "r")
    if f then
        f:close()
        return true
    end
    return false
end

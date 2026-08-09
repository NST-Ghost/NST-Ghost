-- Mock Data Plugin
-- เพิ่ม mock data สำหรับ testing

local mock_data = {
    ["Hello"] = "สวัสดี",
    ["Start Game"] = "เริ่มเกม",
    ["Load Game"] = "โหลดเกม",
    ["Settings"] = "ตั้งค่า",
    ["Exit"] = "ออก"
}

function on_install()
    nst_log("Mock Data Plugin - No installation needed")
    return true
end

function get_menu_items()
    return "Load Mock Data"
end

function on_menu_click()
    nst_log("Loading mock translations...")
    for en, th in pairs(mock_data) do
        nst_log(en .. " -> " .. th)
    end
end

function on_text_extract(text)
    -- Return mock translation if available
    if mock_data[text] then
        return mock_data[text]
    end
    return text
end

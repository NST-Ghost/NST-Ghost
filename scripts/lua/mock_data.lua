-- Mock Data Plugin
-- แสดง mock translations ในตาราง

local mock_translations = {
    {source = "Hello", translation = "สวัสดี"},
    {source = "Goodbye", translation = "ลาก่อน"},
    {source = "Start Game", translation = "เริ่มเกม"},
    {source = "Load Game", translation = "โหลดเกม"},
    {source = "Save Game", translation = "บันทึกเกม"},
    {source = "Settings", translation = "ตั้งค่า"},
    {source = "Exit", translation = "ออก"},
    {source = "New Game", translation = "เกมใหม่"},
    {source = "Continue", translation = "ดำเนินการต่อ"},
    {source = "Options", translation = "ตัวเลือก"}
}

function on_install()
    nst_log("Mock Data Plugin - ไม่ต้องติดตั้งอะไร")
    return true
end

function get_menu_items()
    return "Load Mock Translations"
end

function on_menu_click()
    nst_log("Loading " .. #mock_translations .. " mock translations into table...")
    
    -- ส่งข้อมูลไปแสดงในตาราง
    nst_load_mock_data(mock_translations)
    
    nst_log("✓ Mock data loaded successfully")
end

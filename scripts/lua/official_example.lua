-- Official Example Script for NST Translation Plugin
-- This script demonstrates how to create a translation plugin using Lua.

-- Metadata (Optional but recommended)
-- Name: Official Example Translator
-- Version: 1.0
-- Author: NST Team

-- The 'on_text_extract' function is the core of the translation plugin.
-- It is called whenever the application needs to translate a piece of text.
--
-- Parameters:
--   text (string): The source text to be translated.
--
-- Returns:
--   string: The translated text. If no translation is found, return the original text.

function on_text_extract(text)
    -- Example: Simple dictionary-based translation
    local dictionary = {
        ["Hello"] = "สวัสดี (Official)",
        ["World"] = "โลก",
        ["Game Start"] = "เริ่มเกม",
        ["Options"] = "ตัวเลือก",
        ["Quit"] = "ออกเกม"
    }

    -- Check if the text exists in our dictionary
    if dictionary[text] then
        return dictionary[text]
    end

    -- Example: Pattern matching replacement
    -- If the text starts with "Level", translate it to "ด่าน"
    local level_num = text:match("^Level (%d+)$")
    if level_num then
        return "ด่าน " .. level_num
    end

    -- If no translation is available, return the original text
    return text
end

-- Optional: 'on_install' is called when the user installs the plugin via the Plugin Manager.
-- Use this to perform any setup, such as downloading resources or initializing databases.
function on_install()
    nst_log("Official Example Plugin installed successfully.")
    return true
end

-- Optional: 'get_menu_items' allows adding a custom menu item to the plugin menu.
-- Returns the name of the menu item.
function get_menu_items()
    return "Show Info"
end

-- Optional: 'on_menu_click' is called when the user clicks the custom menu item.
function on_menu_click()
    nst_log("This is the official example plugin for NST.")
    nst_log("It demonstrates dictionary lookups and pattern matching.")
end

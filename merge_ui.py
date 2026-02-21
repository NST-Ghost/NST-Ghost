import xml.etree.ElementTree as ET

def merge_google_modes(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Find translatorModeComboBox and update its items
    combo_box = None
    for widget in root.iter('widget'):
        if widget.get('name') == 'translatorModeComboBox':
            combo_box = widget
            break

    if combo_box is not None:
        # Remove all existing items
        for item in list(combo_box):
            if item.tag == 'item':
                combo_box.remove(item)
        
        # Add new merged items
        for text in ['Google Translate', 'AI-Powered (LLM)', 'Plugins (Lua)']:
            item = ET.SubElement(combo_box, 'item')
            prop = ET.SubElement(item, 'property', {'name': 'text'})
            string_elem = ET.SubElement(prop, 'string')
            string_elem.text = text

    # Find engineStackedWidget
    stacked_widget = None
    for widget in root.iter('widget'):
        if widget.get('name') == 'engineStackedWidget':
            stacked_widget = widget
            break

    if stacked_widget is not None:
        # Find the quick page and remove it
        quick_page = None
        pro_page = None
        for page in stacked_widget.findall('widget'):
            if page.get('name') == 'page_engine_quick':
                quick_page = page
            elif page.get('name') == 'page_engine_pro':
                pro_page = page

        if quick_page is not None:
            stacked_widget.remove(quick_page)

        # Rename pro page to google page
        if pro_page is not None:
            pro_page.set('name', 'page_engine_google')
            # Add label indicating fallback to free engine
            layout = pro_page.find('layout')
            if layout is not None:
                # Create a new item with the label
                item_elem = ET.Element('item')
                label_widget = ET.SubElement(item_elem, 'widget', {'class': 'QLabel', 'name': 'label_google_fallback'})
                prop_elem = ET.SubElement(label_widget, 'property', {'name': 'text'})
                string_elem = ET.SubElement(prop_elem, 'string')
                string_elem.text = 'Leave API Key blank to use the free Google engine.'
                
                # Insert at the top
                layout.insert(4, item_elem) # 4 properties already exist

    ET.indent(tree, space=" ", level=0)
    tree.write(file_path, encoding='UTF-8', xml_declaration=True)

if __name__ == "__main__":
    merge_google_modes("/home/jop/work/NST/NST/src/dialogs/settingsdialog.ui")
    print("Done merging UI.")

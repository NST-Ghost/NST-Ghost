import xml.etree.ElementTree as ET

def add_openai(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Find llmProviderComboBox and add OpenAI
    combo_box = None
    for widget in root.iter('widget'):
        if widget.get('name') == 'llmProviderComboBox':
            combo_box = widget
            break

    if combo_box is not None:
        # Create OpenAI item
        item = ET.Element('item')
        prop = ET.SubElement(item, 'property', {'name': 'text'})
        string_elem = ET.SubElement(prop, 'string')
        string_elem.text = 'OpenAI'
        
        # Insert at the top (index 0)
        combo_box.insert(0, item)

    ET.indent(tree, space=" ", level=0)
    tree.write(file_path, encoding='UTF-8', xml_declaration=True)

if __name__ == "__main__":
    add_openai("/home/jop/work/NST/NST/src/dialogs/settingsdialog.ui")
    print("Done adding OpenAI to UI.")

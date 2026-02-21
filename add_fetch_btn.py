import xml.etree.ElementTree as ET

def add_fetch_models_ui(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Find the Advanced group box to insert the fetch button before it 
    # Or better, we can change the layout of llmModelLabel and combobox
    # to be a QHBoxLayout so we can add a button next to the combobox.
    
    form_layout = None
    for layout in root.iter('layout'):
        if layout.get('name') == 'formLayout_ai_powered':
            form_layout = layout
            break

    if form_layout is not None:
        model_combo_item = None
        for item in list(form_layout):
            if item.tag == 'item' and item.get('row') == '2' and item.get('column') == '1':
                model_combo_item = item
                break

        if model_combo_item is not None:
            # We want to replace the QComboBox with a layout containing the combo and a button
            combo_widget = model_combo_item.find('widget')
            
            # Create HBoxLayout
            hbox_layout = ET.Element('layout', {'class': 'QHBoxLayout', 'name': 'horizontalLayout_model'})
            
            # Item for Combo
            combo_item = ET.SubElement(hbox_layout, 'item')
            combo_item.append(combo_widget)
            
            # Item for Button
            btn_item = ET.SubElement(hbox_layout, 'item')
            btn_widget = ET.SubElement(btn_item, 'widget', {'class': 'QPushButton', 'name': 'fetchModelsButton'})
            prop1 = ET.SubElement(btn_widget, 'property', {'name': 'text'})
            string1 = ET.SubElement(prop1, 'string')
            string1.text = 'Fetch'
            prop2 = ET.SubElement(btn_widget, 'property', {'name': 'toolTip'})
            string2 = ET.SubElement(prop2, 'string')
            string2.text = 'Fetch available models from the provider using the API key'
            
            # Replace widget with layout in the form layout item
            model_combo_item.remove(combo_widget)
            model_combo_item.append(hbox_layout)

    ET.indent(tree, space=" ", level=0)
    tree.write(file_path, encoding='UTF-8', xml_declaration=True)

if __name__ == "__main__":
    add_fetch_models_ui("/home/jop/work/NST/NST/src/dialogs/settingsdialog.ui")
    print("Done expanding UI for fetch models.")

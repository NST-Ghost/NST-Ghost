import xml.etree.ElementTree as ET

def refactor_ui(file_path):
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Find page_general and page_translation
    page_general = None
    page_translation = None
    
    for widget in root.iter('widget'):
        if widget.get('name') == 'page_general':
            page_general = widget
        elif widget.get('name') == 'page_translation':
            page_translation = widget

    # Find the items in general
    formLayout_general = None
    for layout in page_general.iter('layout'):
        if layout.get('name') == 'formLayout_general':
            formLayout_general = layout
            break

    label_item = None
    combo_item = None
    for item in list(formLayout_general):
        if item.tag == 'item':
            row = item.get('row')
            col = item.get('column')
            if row == '2' and col == '0':
                label_item = item
            elif row == '2' and col == '1':
                combo_item = item
            # Shift row 3 and 4 up
            elif row in ['3', '4']:
                item.set('row', str(int(row) - 1))

    # Remove them from general
    formLayout_general.remove(label_item)
    formLayout_general.remove(combo_item)
    label_item.set('row', '0')
    combo_item.set('row', '0')

    # Find scrollArea inside translation
    vlayout_scroll = None
    for layout in page_translation.iter('layout'):
        if layout.get('name') == 'verticalLayout_scroll_translation':
            vlayout_scroll = layout
            break

    # Extract group boxes
    pro_group = None
    llm_group = None
    spacer_item = None
    
    items_to_remove = []
    for item in list(vlayout_scroll):
        if item.tag == 'item':
            widget = item.find('widget')
            if widget is not None:
                if widget.get('name') == 'googleProGroupBox':
                    pro_group = item
                    items_to_remove.append(item)
                elif widget.get('name') == 'llmProviderGroup':
                    llm_group = item
                    items_to_remove.append(item)
            elif item.find('spacer') is not None:
                spacer_item = item
                items_to_remove.append(item)
                
    for item in items_to_remove:
        vlayout_scroll.remove(item)

    # Create top group box for active mode
    top_group_item = ET.Element('item')
    top_group_widget = ET.SubElement(top_group_item, 'widget', {'class': 'QGroupBox', 'name': 'translationModeGroupBox'})
    top_group_title = ET.SubElement(top_group_widget, 'property', {'name': 'title'})
    top_group_string = ET.SubElement(top_group_title, 'string')
    top_group_string.text = 'Translation Mode'
    
    top_layout = ET.SubElement(top_group_widget, 'layout', {'class': 'QFormLayout', 'name': 'formLayout_translationMode'})
    top_layout.append(label_item)
    top_layout.append(combo_item)

    vlayout_scroll.append(top_group_item)

    # Create Stacked Widget
    stacked_item = ET.Element('item')
    stacked_widget = ET.SubElement(stacked_item, 'widget', {'class': 'QStackedWidget', 'name': 'engineStackedWidget'})
    stacked_idx = ET.SubElement(stacked_widget, 'property', {'name': 'currentIndex'})
    stacked_idx_val = ET.SubElement(stacked_idx, 'number')
    stacked_idx_val.text = '0'

    # Page 0: Quick
    page0 = ET.SubElement(stacked_widget, 'widget', {'class': 'QWidget', 'name': 'page_engine_quick'})
    l0 = ET.SubElement(page0, 'layout', {'class': 'QVBoxLayout', 'name': 'verticalLayout_engine_quick'})
    i0 = ET.SubElement(l0, 'item')
    w0 = ET.SubElement(i0, 'widget', {'class': 'QLabel', 'name': 'label_engine_quick'})
    p0 = ET.SubElement(w0, 'property', {'name': 'text'})
    s0 = ET.SubElement(p0, 'string')
    s0.text = 'Google Free engine does not require any configuration.'
    
    sp0_i = ET.SubElement(l0, 'item')
    sp0 = ET.SubElement(sp0_i, 'spacer', {'name': 'spacer_engine_quick'})
    sp0_prop1 = ET.SubElement(sp0, 'property', {'name': 'orientation'})
    ET.SubElement(sp0_prop1, 'enum').text = 'Qt::Vertical'

    # Page 1: Pro
    page1 = ET.SubElement(stacked_widget, 'widget', {'class': 'QWidget', 'name': 'page_engine_pro'})
    l1 = ET.SubElement(page1, 'layout', {'class': 'QVBoxLayout', 'name': 'verticalLayout_engine_pro'})
    # Set margins to 0
    for prop in ['leftMargin', 'topMargin', 'rightMargin', 'bottomMargin']:
        ET.SubElement(l1, 'property', {'name': prop}).append(ET.Element('number', text='0'))
    l1.append(pro_group)
    
    sp1_i = ET.SubElement(l1, 'item')
    sp1 = ET.SubElement(sp1_i, 'spacer', {'name': 'spacer_engine_pro'})
    sp1_prop1 = ET.SubElement(sp1, 'property', {'name': 'orientation'})
    ET.SubElement(sp1_prop1, 'enum').text = 'Qt::Vertical'

    # Page 2: LLM
    page2 = ET.SubElement(stacked_widget, 'widget', {'class': 'QWidget', 'name': 'page_engine_llm'})
    l2 = ET.SubElement(page2, 'layout', {'class': 'QVBoxLayout', 'name': 'verticalLayout_engine_llm'})
    for prop in ['leftMargin', 'topMargin', 'rightMargin', 'bottomMargin']:
        ET.SubElement(l2, 'property', {'name': prop}).append(ET.Element('number', text='0'))
    l2.append(llm_group)

    sp2_i = ET.SubElement(l2, 'item')
    sp2 = ET.SubElement(sp2_i, 'spacer', {'name': 'spacer_engine_llm'})
    sp2_prop1 = ET.SubElement(sp2, 'property', {'name': 'orientation'})
    ET.SubElement(sp2_prop1, 'enum').text = 'Qt::Vertical'

    # Page 3: Plugins
    page3 = ET.SubElement(stacked_widget, 'widget', {'class': 'QWidget', 'name': 'page_engine_plugins'})
    l3 = ET.SubElement(page3, 'layout', {'class': 'QVBoxLayout', 'name': 'verticalLayout_engine_plugins'})
    i3 = ET.SubElement(l3, 'item')
    w3 = ET.SubElement(i3, 'widget', {'class': 'QLabel', 'name': 'label_engine_plugins'})
    p3 = ET.SubElement(w3, 'property', {'name': 'text'})
    s3 = ET.SubElement(p3, 'string')
    s3.text = 'Plugin scripts manage their own configuration. Go to the Plugins tab.'
    
    sp3_i = ET.SubElement(l3, 'item')
    sp3 = ET.SubElement(sp3_i, 'spacer', {'name': 'spacer_engine_plugins'})
    sp3_prop1 = ET.SubElement(sp3, 'property', {'name': 'orientation'})
    ET.SubElement(sp3_prop1, 'enum').text = 'Qt::Vertical'

    vlayout_scroll.append(stacked_item)
    if spacer_item is not None:
        vlayout_scroll.append(spacer_item)

    ET.indent(tree, space=" ", level=0)
    tree.write(file_path, encoding='UTF-8', xml_declaration=True)

if __name__ == "__main__":
    refactor_ui("/home/jop/work/NST/NST/src/dialogs/settingsdialog.ui")
    print("Done refactoring UI.")

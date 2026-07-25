use bga_rs::engines::godot::GodotAnalyzer;
use bga_rs::GameAnalyzer;
use std::fs;
use std::path::Path;

#[test]
fn test_godot_extraction_mock_project() {
    let mock_dir = Path::new("/tmp/nst_godot_test_project");
    let _ = fs::remove_dir_all(mock_dir);
    fs::create_dir_all(mock_dir.join("scenes")).unwrap();
    fs::create_dir_all(mock_dir.join("scripts")).unwrap();

    // 1. Create a scene file (.tscn)
    fs::write(
        mock_dir.join("scenes/main.tscn"),
        r#"[gd_scene format=3]
[node name="Main" type="Control"]
[node name="Label" type="Label" parent="."]
text = "こんにちは、世界！"
dialog_text = "これはテストダイアログです。"
"#,
    ).unwrap();

    // 2. Create a script file (.gd)
    fs::write(
        mock_dir.join("scripts/dialogue.gd"),
        r#"extends Node

func start_dialogue():
    var name = "小春"
    var line = "「おはよう、蓮くん！」"
    print(tr("KEY_WELCOME"))
"#,
    ).unwrap();

    // 3. Create a CSV translation file
    fs::write(
        mock_dir.join("translations.csv"),
        "keys,en,ja\nKEY_WELCOME,\"Welcome to the game!\",\"ゲームへようこそ！\"\nKEY_QUIT,\"Quit Game\",\"ゲーム終了\"\n",
    ).unwrap();

    let analyzer = GodotAnalyzer::new();
    let result = analyzer.analyze(mock_dir);

    assert!(result.error_message.is_none(), "Expected success, got error: {:?}", result.error_message);
    assert!(!result.payload.is_empty(), "Payload should not be empty");

    let json: serde_json::Value = serde_json::from_str(&result.payload).unwrap();
    let strings = json["strings"].as_array().expect("Expected strings array");

    println!("Extracted {} strings from mock Godot project", strings.len());
    assert!(strings.len() >= 4, "Should extract at least 4 strings");

    // Clean up
    let _ = fs::remove_dir_all(mock_dir);
}

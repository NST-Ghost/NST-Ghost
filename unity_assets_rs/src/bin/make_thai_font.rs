use std::fs;
use unity_assets_rs::create_legacy_font_from_template;

fn main() -> anyhow::Result<()> {
    // 1. ตรวจสอบไฟล์ที่จำเป็น
    let template_path = "template.font";
    let ttf_path = "sarabun.ttf";
    let output_path = "Sarabun_Unity.font";

    if !std::path::Path::new(template_path).exists() {
        println!("[ERROR] ไม่พบไฟล์ '{}'", template_path);
        println!("กรุณาสร้าง Template จาก Unity ตามขั้นตอนที่คุณแนะนำก่อนครับ (ลาก TTF เข้า Assets แล้วเอาไฟล์ .font มาวาง)");
        return Ok(());
    }

    println!("[READ] Reading template: {}...", template_path);
    let template_data = fs::read(template_path)?;
    
    println!("Font data: {}...", ttf_path);
    let ttf_data = fs::read(ttf_path)?;

    println!("[INFO] Creating Unity Font Asset...");
    let asset = create_legacy_font_from_template(
        &template_data,
        &ttf_data,
        "Sarabun-Regular", // ชื่อที่จะปรากฏใน Unity
    )?;

    fs::write(output_path, asset)?;
    println!("[SUCCESS] สร้างไฟล์ '{}' สำเร็จแล้ว!", output_path);
    println!("คุณสามารถนำไฟล์นี้ไปใช้กับคำสั่ง font_overwrite เพื่อยัดลงเกมได้เลยครับ");

    Ok(())
}

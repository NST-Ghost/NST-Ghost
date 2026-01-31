use clap::Parser;
use std::path::PathBuf;
use anyhow::{Context, Result};
use unrpyc_rs::reader::{read_rpyc_file, decompress_data};

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Input file path (e.g., script.rpyc)
    #[arg(required = true)]
    file: PathBuf,

    /// Dump internal structure instead of full decompilation
    #[arg(short, long)]
    dump: bool,
}

fn main() -> Result<()> {
    let args = Args::parse();

    println!("Processing file: {:?}", args.file);

    // 1. Read file
    let raw_data = read_rpyc_file(&args.file).context("Failed to read rpyc file")?;
    println!("Read {} bytes of raw data (or extracted slot 1)", raw_data.len());

    // 2. Decompress
    let decompressed = decompress_data(&raw_data).context("Failed to decompress data")?;
    println!("Decompressed to {} bytes", decompressed.len());

    // 3. Unpickle
    // We try to decode as a generic Value first to inspect structure
    let options = serde_pickle::DeOptions::new().replace_unresolved_globals();
    let decoded: serde_pickle::Value = serde_pickle::from_slice(&decompressed, options)
        .context("Failed to unpickle data")?;

    if args.dump {
        println!("{:#?}", decoded);
    } else {
        println!("Successfully unpickled data. Structure available.");
        // TODO: Map to AST
    }

    Ok(())
}

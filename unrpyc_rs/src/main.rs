use clap::{Parser, Subcommand};
use std::path::{PathBuf, Path};
use anyhow::{Context, Result};
use unrpyc_rs::reader::{read_rpyc_file, decompress_data};

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    command: Option<Commands>,

    /// Input file (.rpyc)
    input: Option<String>,

    /// Dump internal structure
    #[arg(long)]
    dump: bool,
}

#[derive(Subcommand, Debug)]
enum Commands {
    /// Extract RPA archive
    Extract {
        /// Archive file (.rpa)
        archive: String,
        /// Output directory (optional)
        #[arg(short, long)]
        output: Option<String>,
    },
}

fn main() -> Result<()> {
    let args = Args::parse();

    if let Some(Commands::Extract { archive, output }) = args.command {
        let rpa = unrpyc_rs::rpa::RpaArchive::open(&archive)?;
        println!("Opened archive: {}", archive);
        let files = rpa.list_files();
        println!("Found {} files.", files.len());
        
        let out_dir = output.unwrap_or_else(|| {
            let path = Path::new(&archive);
            path.file_stem().unwrap().to_string_lossy().to_string() + "_extracted"
        });
        
        std::fs::create_dir_all(&out_dir)?;
        
        let mut rpa = rpa; // Make mutable for extraction
        for file in files {
            println!("Extracting {}", file);
            if let Some(data) = rpa.extract_file(&file)? {
                let out_path = Path::new(&out_dir).join(&file);
                if let Some(parent) = out_path.parent() {
                    std::fs::create_dir_all(parent)?;
                }
                std::fs::write(out_path, data)?;
            }
        }
        println!("Extraction complete to {}", out_dir);
        return Ok(());
    }

    // Default behavior: Decompile .rpyc
    // Manually ensure input is provided if not a subcommand
    let input_path = match args.input {
        Some(p) => p,
        None => {
             use clap::CommandFactory;
             Args::command().print_help()?;
             std::process::exit(1);
        }
    };
    
    println!("Processing file: {:?}", input_path);

    // 1. Read file
    let raw_data = read_rpyc_file(&PathBuf::from(&input_path)).context("Failed to read rpyc file")?;
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
        println!("Successfully unpickled data. Mapping to AST...");
        if let Some(stmts) = unrpyc_rs::ast::extract_statements(&decoded) {
             println!("Found {} statements", stmts.len());
             for (i, stmt) in stmts.iter().enumerate() {
                 match unrpyc_rs::ast::parse_statement(stmt) {
                     Ok(parsed) => println!("Stmt {}: {:?}", i, parsed),
                     Err(e) => println!("Stmt {}: Failed: {}", i, e),
                 }
             }
        } else {
             println!("Could not extract statements from parsed value.");
        }
    }

    Ok(())
}

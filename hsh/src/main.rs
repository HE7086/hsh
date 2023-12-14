use hsh_parse;

use clap::Parser;

#[derive(Parser)]
#[clap(trailing_var_arg=true)]
#[command(version)]
struct Args {

}

fn main() {
    let args = Args::parse();
    println!("Hello, world!");
}

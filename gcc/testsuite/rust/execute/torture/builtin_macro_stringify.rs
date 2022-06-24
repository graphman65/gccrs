// { dg-output "let a = 2\n18\n" }
extern "C" {
    fn printf(fmt: *const i8, ...);
}

fn print(s: &str) {
    printf(
        "%s\n" as *const str as *const i8,
        s as *const str as *const i8,
    );
}

macro_rules! stringify {
    ($($t:tt)*) => {};
}

fn main() -> i32 {
    let a = stringify!(let a = 2);
    print(a);

    let b = stringify!({
        testing stringify
        for i := range 1...123 {
            println!("go go go")
        }
        let x = 123;
    });
    print(b);

    0
}

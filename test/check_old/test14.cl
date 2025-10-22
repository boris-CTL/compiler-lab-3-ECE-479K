class Main {
    main(): Int {
        let x : Int <- 2 in {
            while x < 1000 loop{
                x <- (x * 2) - 1;
            } pool;
            x;
        }
    };
};
class Main {
    main(): Int {
        let x : Int <- 0 in {
            while x < 10 loop{
                x <- x + 1;
            } pool;
            x;
        }
    };
};
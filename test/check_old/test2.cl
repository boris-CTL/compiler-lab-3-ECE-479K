class Main {
    main(): Int {
        let x : Int <- 2 in {
            while x < 1000 loop{
                x <- (x * 2) - 1;
            } pool;

            while 0 < x loop{
                x <- x - 1;
            } pool;
            x;
        }
    };
};
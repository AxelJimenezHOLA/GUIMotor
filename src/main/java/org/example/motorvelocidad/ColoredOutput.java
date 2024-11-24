package org.example.motorvelocidad;

public class ColoredOutput {

    public enum ANSIColor {
        RESET("\u001B[0m"),
        BLACK("\u001B[30m"),
        RED("\u001B[31m"),
        GREEN("\u001B[32m"),
        YELLOW("\u001B[33m"),
        BLUE("\u001B[34m"),
        PURPLE("\u001B[35m"),
        CYAN("\u001B[36m"),
        WHITE("\u001B[37m");

        private final String code;

        ANSIColor(String code) {
            this.code = code;
        }

        @Override
        public String toString() {
            return code;
        }
    }

    public static void colorPrintln(String s, ANSIColor color) {
        System.out.println(color + s + ANSIColor.RESET);
    }

    public static void colorPrint(String s, ANSIColor color) {
        System.out.print(color + s + ANSIColor.RESET);
    }
}
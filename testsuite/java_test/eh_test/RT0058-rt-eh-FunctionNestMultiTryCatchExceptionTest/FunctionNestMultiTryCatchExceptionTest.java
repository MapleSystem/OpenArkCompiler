/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.io.PrintStream;
import java.nio.charset.IllegalCharsetNameException;
import java.util.IllegalFormatException;
public class FunctionNestMultiTryCatchExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = run1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 0 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Nested run1 fun
     * @return status code
    */

    public static int run1() {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = run2();
        } catch (IllegalArgumentException e) {
            processResult -= 1;
        }
        if (processResult == 98) {
            result = 0;
            processResult -= 3;
        }
        return result;
    }
    /**
     * Nested run2 fun
     * @return status code
    */

    public static int run2() {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = run3();
        } catch (ClassCastException e) {
            processResult -= 2;
        }
        if (processResult == 97) {
            result = 0;
        }
        return result;
    }
    /**
     * Nested run3 fun
     * @return status code
    */

    public static int run3() {
        int result = 3; /* STATUS_FAILED*/

        try {
            result = run4();
        } catch (IllegalStateException e) {
            processResult -= 3;
        }
        if (processResult == 96) {
            result = 0;
        }
        return result;
    }
    /**
     * Nested run4 fun
     * @return status code
    */

    public static int run4() {
        int result = 4; /* STATUS_FAILED*/

        try {
            result = run5();
        } catch (IllegalFormatException e) {
            processResult -= 4;
        }
        if (processResult == 95) {
            result = 0;
        }
        return result;
    }
    /**
     * Nested run5 fun
     * @return status code
    */

    public static int run5() {
        int result = 5; /* STATUS_FAILED*/

        try {
            result = functionNestMultiTryCatchExceptionTest1();
        } catch (IllegalCharsetNameException e) {
            processResult -= 5;
        }
        if (processResult == 94) {
            result = 0;
        }
        return result;
    }
    /**
     * Test try{try{try{try{try{exception}catch()}catch()}catch()}catch()}, -->caller.caller.caller
     * @return status code
    */

    public static int functionNestMultiTryCatchExceptionTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        String str = "123#456";
        try {
            processResult += 100;
            try {
                processResult += 100;
                try {
                    processResult += 100;
                    try {
                        processResult += 100;
                        try {
                            processResult += 100;
                            Integer.parseInt(str);
                        } catch (ClassCastException e) {
                            processResult -= 10;
                        } finally {
                            processResult -= 100;
                        }
                    } catch (ClassCastException e) {
                        processResult -= 10;
                    } finally {
                        processResult -= 100;
                    }
                } catch (ClassCastException e) {
                    processResult -= 10;
                } finally {
                    processResult -= 100;
                }
            } catch (IllegalStateException e) {
                processResult -= 10;
            } finally {
                processResult -= 100;
            }
            processResult -= 10;
        } catch (IndexOutOfBoundsException e) {
            processResult -= 10;
        } catch (IllegalCharsetNameException e) {
            processResult -= 10;
        } finally {
            processResult -= 100;
        }
        processResult -= 10;
        return result1;
    }
}

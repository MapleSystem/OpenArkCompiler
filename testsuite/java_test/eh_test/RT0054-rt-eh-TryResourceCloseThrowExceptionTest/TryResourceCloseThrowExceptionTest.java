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
public class TryResourceCloseThrowExceptionTest {
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
            result = tryResourceCloseThrowExceptionTest1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 1 && processResult == 94) {
            result = 0;
        }
        return result;
    }
    public static class MyResource implements AutoCloseable {
        /**
         * start fun
         * @throws NumberFormatException
        */

        public void start() throws NumberFormatException {
            if (processResult == 99) {
                processResult--;
            }
        }
        @Override
        public void close() throws StringIndexOutOfBoundsException {
            if (processResult == 98) {
                processResult--;
            }
            throw new StringIndexOutOfBoundsException("end:exception!!!");
        }
    }
    /**
     * Test try(){}-catch(e){}-close(exception)-finally.
     * @return status code
    */

    public static int tryResourceCloseThrowExceptionTest1() {
        int result1 = 4; /* STATUS_FAILED*/

        try (MyResource conn = new MyResource()) {
            conn.start();
        } catch (NumberFormatException e) {
            processResult -= 10;
            e.printStackTrace();
            result1 = 3;
        } catch (StringIndexOutOfBoundsException e) {
            if (processResult == 97) {
                processResult--;
            }
            result1 = 1;
        } finally {
            if (processResult == 96) {
                processResult--;
            }
        }
        if (processResult == 95) {
            processResult--;
        }
        return result1;
    }
}

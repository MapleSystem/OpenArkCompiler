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
public class StringConsBytesIntIntIntAndBytesTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringConsBytesIntIntIntAndBytesTest_1();
            StringConsBytesIntIntIntAndBytesTest_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    // Test constructor String(byte[] ascii, int hibyte, int offset, int count).
    public static void StringConsBytesIntIntIntAndBytesTest_1() {
        byte[] ascii1_1 = new byte[]{(byte) 0x61, (byte) 0x62, (byte) 0x63, (byte) 0x31, (byte) 0x32, (byte) 0x33};
        String str1 = new String(ascii1_1, 0, 0, 2);
        String str1_1 = new String(ascii1_1, 0, 0, 6);
        String str1_2 = new String(ascii1_1, 0, 0, 0);
        String str1_3 = new String(ascii1_1, 0, 5, 1);
        String str1_4 = new String(ascii1_1, 1, 5, 1);
        System.out.println(str1);
        System.out.println(str1_1);
        System.out.println(str1_2);
        System.out.println(str1_3);
        System.out.println(str1_4);
    }
    // Test constructor String(byte[] bytes).
    public static void StringConsBytesIntIntIntAndBytesTest_2() {
        byte[] ascii1_1 = new byte[]{(byte) 0x61, (byte) 0x62, (byte) 0x63, (byte) 0x31, (byte) 0x32, (byte) 0x33};
        String str0 = new String(ascii1_1);
        System.out.println(str0);
    }
}

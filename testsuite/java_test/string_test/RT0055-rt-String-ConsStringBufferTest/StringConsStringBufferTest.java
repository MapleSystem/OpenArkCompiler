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
public class StringConsStringBufferTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringConsStringBufferTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringConsStringBufferTest_1() {
        String str1 = "These " + "are " + "abcdefghijklmnopqrstuvwxyz";
        String str2_1 = "These ";
        String str2_2 = "are ";
        String str2_3 = "abcdefghijklmnopqrstuvwxyz";
        String str2 = str2_1 + str2_2 + str2_3;
        StringBuffer str_b_1 = new StringBuffer("These ").append("are ").append("abcdefghijklmnopqrstuvwxyz");
        String str3 = new String(str_b_1);
        System.out.println(str1);
        System.out.println(str2);
        System.out.println(str3);
    }
}

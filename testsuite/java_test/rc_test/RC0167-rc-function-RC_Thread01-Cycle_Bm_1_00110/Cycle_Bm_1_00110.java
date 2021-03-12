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


class ThreadRc_Cycle_Bm_1_00110 extends Thread {
    private boolean checkout;
    public void run() {
        Cycle_B_1_00110_A1 a1_0 = new Cycle_B_1_00110_A1();
        a1_0.a2_0 = new Cycle_B_1_00110_A2();
        a1_0.a2_0.a3_0 = new Cycle_B_1_00110_A3();
        Cycle_B_1_00110_A5 a5_0 = new Cycle_B_1_00110_A5();
        a5_0.a4_0 = new Cycle_B_1_00110_A4();
        a5_0.a4_0.a2_0 = a1_0.a2_0;
        a1_0.a2_0.a3_0.a1_0 = a1_0;
        a1_0.add();
        a1_0.a2_0.add();
        a1_0.a2_0.a3_0.add();
        a5_0.a4_0.add();
        a5_0.add();
        int nsum = a1_0.sum + a1_0.a2_0.sum + a1_0.a2_0.a3_0.sum + a5_0.a4_0.sum + a5_0.sum;
        //System.out.println(nsum);
        if (nsum == 27)
            checkout = true;
        //System.out.println(checkout);
    }
    public boolean check() {
        return checkout;
    }
    class Cycle_B_1_00110_A1 {
        Cycle_B_1_00110_A2 a2_0;
        int a;
        int sum;
        Cycle_B_1_00110_A1() {
            a2_0 = null;
            a = 1;
            sum = 0;
        }
        void add() {
            sum = a + a2_0.a;
        }
    }
    class Cycle_B_1_00110_A2 {
        Cycle_B_1_00110_A3 a3_0;
        int a;
        int sum;
        Cycle_B_1_00110_A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
        void add() {
            sum = a + a3_0.a;
        }
    }
    class Cycle_B_1_00110_A3 {
        Cycle_B_1_00110_A1 a1_0;
        int a;
        int sum;
        Cycle_B_1_00110_A3() {
            a1_0 = null;
            a = 3;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a;
        }
    }
    class Cycle_B_1_00110_A4 {
        Cycle_B_1_00110_A2 a2_0;
        int a;
        int sum;
        Cycle_B_1_00110_A4() {
            a2_0 = null;
            a = 4;
            sum = 0;
        }
        void add() {
            sum = a + a2_0.a;
        }
    }
    class Cycle_B_1_00110_A5 {
        Cycle_B_1_00110_A4 a4_0;
        int a;
        int sum;
        Cycle_B_1_00110_A5() {
            a4_0 = null;
            a = 5;
            sum = 0;
        }
        void add() {
            sum = a + a4_0.a;
        }
    }
}
public class Cycle_Bm_1_00110 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        ThreadRc_Cycle_Bm_1_00110 A1_Cycle_Bm_1_00110 = new ThreadRc_Cycle_Bm_1_00110();
        ThreadRc_Cycle_Bm_1_00110 A2_Cycle_Bm_1_00110 = new ThreadRc_Cycle_Bm_1_00110();
        ThreadRc_Cycle_Bm_1_00110 A3_Cycle_Bm_1_00110 = new ThreadRc_Cycle_Bm_1_00110();
        ThreadRc_Cycle_Bm_1_00110 A4_Cycle_Bm_1_00110 = new ThreadRc_Cycle_Bm_1_00110();
        ThreadRc_Cycle_Bm_1_00110 A5_Cycle_Bm_1_00110 = new ThreadRc_Cycle_Bm_1_00110();
        ThreadRc_Cycle_Bm_1_00110 A6_Cycle_Bm_1_00110 = new ThreadRc_Cycle_Bm_1_00110();
        A1_Cycle_Bm_1_00110.start();
        A2_Cycle_Bm_1_00110.start();
        A3_Cycle_Bm_1_00110.start();
        A4_Cycle_Bm_1_00110.start();
        A5_Cycle_Bm_1_00110.start();
        A6_Cycle_Bm_1_00110.start();
        try {
            A1_Cycle_Bm_1_00110.join();
            A2_Cycle_Bm_1_00110.join();
            A3_Cycle_Bm_1_00110.join();
            A4_Cycle_Bm_1_00110.join();
            A5_Cycle_Bm_1_00110.join();
            A6_Cycle_Bm_1_00110.join();
        } catch (InterruptedException e) {
        }
        if (A1_Cycle_Bm_1_00110.check() && A2_Cycle_Bm_1_00110.check() && A3_Cycle_Bm_1_00110.check() && A4_Cycle_Bm_1_00110.check() && A5_Cycle_Bm_1_00110.check() && A6_Cycle_Bm_1_00110.check())
            System.out.println("ExpectResult");
    }
}

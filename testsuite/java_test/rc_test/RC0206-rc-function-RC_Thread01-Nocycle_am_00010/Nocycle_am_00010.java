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


class ThreadRc_00010 extends Thread {
    private boolean checkout;
    public void run() {
        Nocycle_am_00010_A1 a1_main = new Nocycle_am_00010_A1("a1_main");
        a1_main.b1_0 = new Nocycle_am_00010_B1("b1_0");
        a1_main.add();
        a1_main.b1_0.add();
//		 System.out.printf("RC-Testing_Result=%d\n",a1_main.sum+a1_main.b1_0.sum);
        int result = a1_main.sum + a1_main.b1_0.sum;
        // System.out.println("RC-Testing_Result_Thread="+result);
        if (result == 704)
            checkout = true;
        //System.out.println(checkout);
    }
    public boolean check() {
        return checkout;
    }
    class Nocycle_am_00010_A1 {
        Nocycle_am_00010_B1 b1_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_am_00010_A1(String strObjectName) {
            b1_0 = null;
            a = 101;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A1_"+strObjectName);
        }
        //   protected void finalize() throws java.lang.Throwable {
//       System.out.println("RC-Testing_Destruction_A1_"+strObjectName);
//   }
        void add() {
            sum = a + b1_0.a;
        }
    }
    class Nocycle_am_00010_B1 {
        int a;
        int sum;
        String strObjectName;
        Nocycle_am_00010_B1(String strObjectName) {
            a = 201;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_B1_"+strObjectName);
        }
        //   protected void finalize() throws java.lang.Throwable {
//       System.out.println("RC-Testing_Destruction_B1_"+strObjectName);
//   }
        void add() {
            sum = a + a;
        }
    }
}
public class Nocycle_am_00010 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        ThreadRc_00010 t_00010 = new ThreadRc_00010();
        t_00010.start();
        ThreadRc_00010 t_00020 = new ThreadRc_00010();
        t_00020.start();
        ThreadRc_00010 t_00030 = new ThreadRc_00010();
        t_00030.start();
        ThreadRc_00010 t_00040 = new ThreadRc_00010();
        t_00040.start();
        ThreadRc_00010 t_00050 = new ThreadRc_00010();
        t_00050.start();
        ThreadRc_00010 t_00060 = new ThreadRc_00010();
        t_00060.start();
        try {
            t_00010.join();
            t_00020.join();
            t_00030.join();
            t_00040.join();
            t_00050.join();
            t_00060.join();
        } catch (InterruptedException e) {
        }
        if (t_00010.check() == true && t_00060.check() == true && t_00020.check() == true && t_00030.check() == true && t_00040.check() == true && t_00050.check() == true)
            System.out.println("ExpectResult");
    }
}
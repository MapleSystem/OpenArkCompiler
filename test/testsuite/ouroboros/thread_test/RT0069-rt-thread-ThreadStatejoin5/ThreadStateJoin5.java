/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 * -@TestCaseID: ThreadStateJoin5
 *- @TestCaseName: Thread_ThreadStateJoin5.java
 *- @RequirementName: Java Thread
 *- @Title/Destination: Wait time unit is millisecond target thread is not finished after wait.
 *- @Brief: see below
 * -#step1: 创建一个ThreadStateJoin5类的实例对象threadStateJoin5，并且ThreadStateJoin5类继承自Thread类；
 * -#step2: 调用threadStateJoin5的start()方法启动该线程；
 * -#step3: 调用threadStateJoin5的join()方法，参数为500、300；
 * -#step4: run方法循环5次，每次令int类型的全局静态变量t的值加1；
 * -#step5: 经判断得知threadStateJoin5.getState().toString()的返回值与字符串"TIMED_WAITING"相同，并且ThreadStateJoin5类
 *          在其内部的run()方法执行完之后t的值变为3；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Source: ThreadStateJoin5.java
 *- @ExecuteClass: ThreadStateJoin5
 *- @ExecuteArgs:
 */

public class ThreadStateJoin5 extends Thread {
    static int t = 0;

    public static void main(String[] args) {
        ThreadStateJoin5 threadStateJoin5 = new ThreadStateJoin5();
        threadStateJoin5.start();
        try {
            threadStateJoin5.join(500, 300);
            t++;
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        if (threadStateJoin5.getState().toString().equals("TIMED_WAITING") && t == 3) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }

    public void run() {
        synchronized (currentThread()) {
            for (int i = 1; i <= 5; i++) {
                try {
                    wait(200);
                    t++;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan 0\n
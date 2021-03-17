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


public class ThreadSetPriority4 extends Thread {
    public static void main(String[] args) {
        ThreadSetPriority4 thread_obj1 = new ThreadSetPriority4();
        ThreadSetPriority4 thread_obj2 = new ThreadSetPriority4();
        ThreadSetPriority4 thread_obj3 = new ThreadSetPriority4();
        thread_obj1.start();
        thread_obj2.start();
        thread_obj3.start();
        try {
            thread_obj1.join();
            thread_obj2.join();
            thread_obj3.join();
        } catch (InterruptedException e) {
        }
        thread_obj1.setPriority(7);
        thread_obj2.setPriority(MAX_PRIORITY);
        thread_obj3.setPriority(MIN_PRIORITY + 1);
        if (thread_obj1.getPriority() != 5) {
            System.out.println(2);
        }
        if (thread_obj2.getPriority() != 5) {
            System.out.println(2);
        }
        if (thread_obj3.getPriority() != 5) {
            System.out.println(2);
        }
        System.out.println(0);
    }
}

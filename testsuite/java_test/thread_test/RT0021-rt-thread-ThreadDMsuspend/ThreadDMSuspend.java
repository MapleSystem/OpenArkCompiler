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


public class ThreadDMSuspend extends Thread {
    static int i = 0; /* STATUS_Success*/

    public static void main(String[] args) {
        ThreadDMSuspend thread_obj = new ThreadDMSuspend();
        thread_obj.start();
        try {
            sleep(50);
        } catch (InterruptedException e2) {
            System.err.println(e2);
        }
        try {
            thread_obj.suspend();
            thread_obj.stop();
        } catch (UnsupportedOperationException e3) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        synchronized (this) {
            try {
                sleep(100);
            } catch (InterruptedException e1) {
                System.err.println(e1);
            }
            i++;
        }
    }
}
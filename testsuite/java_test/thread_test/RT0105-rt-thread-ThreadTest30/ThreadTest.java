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


class ThreadTest {
    public static void main(String[] args) {
        ThreadWaiting t = new ThreadWaiting(5000, 0);
        t.start();
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            System.out.println("main throws InterruptedException");
        }
        t.interrupt();
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            System.out.println("main throws InterruptedException");
        }
        System.out.println("interrupt status has been cleared, if the output is false -- " +
                t.isInterrupted());
    }
    private static class ThreadWaiting extends Thread {
        private long millis;
        private int nanos;
        ThreadWaiting(long millis, int nanos) {
            this.millis = millis;
            this.nanos = nanos;
        }
        public void run() {
            synchronized (this) {
                try {
                    this.wait(millis, nanos);
                    System.out.println("Fail -- waiting thread has not received the InterruptedException");
                } catch (InterruptedException e) {
                    System.out.println("waiting thread has received the InterruptedException");
                }
            }
        }
    }
}
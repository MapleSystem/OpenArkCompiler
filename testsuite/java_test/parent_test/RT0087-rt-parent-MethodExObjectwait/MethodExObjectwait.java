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


import java.lang.reflect.Method;
public class MethodExObjectwait {
    static int res = 99;
    private Method[] cal = MethodExObjectwait.class.getMethods();
    private Method[] cal2 = MethodExObjectwait.class.getMethods();
    private Method[] cal3 = MethodExObjectwait.class.getMethods();
    public static void main(String argv[]) {
        System.out.println(new MethodExObjectwait().run());
    }
    private class MethodExObjectwait11 implements Runnable {
        // final void wait()
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (cal) {
                cal.notifyAll();
                try {
                    cal.wait();
                    MethodExObjectwait.res = MethodExObjectwait.res - 10;
                } catch (InterruptedException e1) {
                    MethodExObjectwait.res = MethodExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    MethodExObjectwait.res = MethodExObjectwait.res - 30;
                }
            }
        }
    }
    private class MethodExObjectwait12 implements Runnable {
        // final void wait(long millis)
        long millis = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (cal2) {
                cal2.notifyAll();
                try {
                    cal2.wait(millis);
                    MethodExObjectwait.res = MethodExObjectwait.res - 10;
                } catch (InterruptedException e1) {
                    MethodExObjectwait.res = MethodExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    MethodExObjectwait.res = MethodExObjectwait.res - 30;
                }
            }
        }
    }
    private class MethodExObjectwait13 implements Runnable {
        // final void wait(long millis, int nanos)
        long millis = 10;
        int nanos = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (cal3) {
                cal3.notifyAll();
                try {
                    cal3.wait(millis, nanos);
                    MethodExObjectwait.res = MethodExObjectwait.res - 10;
                } catch (InterruptedException e1) {
                    MethodExObjectwait.res = MethodExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    MethodExObjectwait.res = MethodExObjectwait.res - 30;
                }
            }
        }
    }
    /**
     * sleep fun
     *
     * @param slpnum wait time
    */

    public void sleep(int slpnum) {
        try {
            Thread.sleep(slpnum);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        // final void wait()
        Thread t1 = new Thread(new MethodExObjectwait11());
        Thread t2 = new Thread(new MethodExObjectwait11());
        // final void wait(long millis)
        Thread t3 = new Thread(new MethodExObjectwait12());
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new MethodExObjectwait13());
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t2.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t3.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t5.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t1.start();
        sleep(1000);
        t3.start();
        sleep(1000);
        t5.start();
        sleep(1000);
        t2.start();
        try {
            t1.join();
            t3.join();
            t5.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (result == 2 && MethodExObjectwait.res == 69) {
            result = 0;
        }
        t2.interrupt();
        return result;
    }
}
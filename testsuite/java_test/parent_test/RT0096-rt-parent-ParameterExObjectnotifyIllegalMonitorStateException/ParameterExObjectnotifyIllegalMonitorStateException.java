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


import java.lang.reflect.Parameter;
import java.lang.reflect.Method;
public class ParameterExObjectnotifyIllegalMonitorStateException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new ParameterExObjectnotifyIllegalMonitorStateException().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = parameterExObjectnotifyIllegalMonitorStateException1();
        } catch (Exception e) {
            ParameterExObjectnotifyIllegalMonitorStateException.res = ParameterExObjectnotifyIllegalMonitorStateException.res - 20;
        }
        Thread t1 = new Thread(new ParameterExObjectnotifyIllegalMonitorStateException11(1));
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t1.start();
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (result == 4 && ParameterExObjectnotifyIllegalMonitorStateException.res == 58) {
            result = 0;
        }
        return result;
    }
    private int parameterExObjectnotifyIllegalMonitorStateException1() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void notify()
        Method method = Parameter02a.class.getMethod("getName", new Class[]{String.class});
        Parameter[] parameters = method.getParameters();
        try {
            parameters[0].notify();
            ParameterExObjectnotifyIllegalMonitorStateException.res = ParameterExObjectnotifyIllegalMonitorStateException.res - 10;
        } catch (IllegalMonitorStateException e2) {
            ParameterExObjectnotifyIllegalMonitorStateException.res = ParameterExObjectnotifyIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private class ParameterExObjectnotifyIllegalMonitorStateException11 implements Runnable {
        // final void notify()
        private int remainder;
        private ParameterExObjectnotifyIllegalMonitorStateException11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            Method me = null;
            try {
                me = Parameter02a.class.getMethod("getName", new Class[]{String.class});
            } catch (NoSuchMethodException | SecurityException e) {
                e.printStackTrace();
            }
            Parameter[] parameters = me.getParameters();
            synchronized (parameters[0]) {
                try {
                    parameters[0].notify();
                    ParameterExObjectnotifyIllegalMonitorStateException.res = ParameterExObjectnotifyIllegalMonitorStateException.res - 40;
                } catch (IllegalMonitorStateException e2) {
                    ParameterExObjectnotifyIllegalMonitorStateException.res = ParameterExObjectnotifyIllegalMonitorStateException.res - 30;
                }
            }
        }
    }
}
class Parameter02a {
    private String name = "default";
    public String getName(String str1) {
        return str1;
    }
}
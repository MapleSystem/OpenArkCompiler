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


import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Constructor;
public class ConstructorExObjectnotifyAllIllegalMonitorStateException {
    static int res = 99;
    private static Constructor<?> con = null;
    public static void main(String argv[]) throws NoSuchMethodException, SecurityException {
        con = SampleClass14.class.getConstructor(String.class);
        System.out.println(new ConstructorExObjectnotifyAllIllegalMonitorStateException().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2;
        try {
            result = constructorExObjectnotifyAllIllegalMonitorStateExceptionTest1();
        } catch (Exception e) {
            ConstructorExObjectnotifyAllIllegalMonitorStateException.res = ConstructorExObjectnotifyAllIllegalMonitorStateException.res - 20;
        }
        Thread t1 = new Thread(new ConstructorExObjectnotifyAllIllegalMonitorStateException11(1));
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
        if (result == 4 && ConstructorExObjectnotifyAllIllegalMonitorStateException.res == 57) {
            result = 0;
        }
        return result;
    }
    private int constructorExObjectnotifyAllIllegalMonitorStateExceptionTest1() {
        int result1 = 4;
        try {
            con.notifyAll();
            ConstructorExObjectnotifyAllIllegalMonitorStateException.res = ConstructorExObjectnotifyAllIllegalMonitorStateException.res - 50;
        } catch (IllegalMonitorStateException e2) {
            ConstructorExObjectnotifyAllIllegalMonitorStateException.res = ConstructorExObjectnotifyAllIllegalMonitorStateException.res - 2;
        }
        return result1;
    }
    private class ConstructorExObjectnotifyAllIllegalMonitorStateException11 implements Runnable {
        private int remainder;
        // final void notifyAll()
        private ConstructorExObjectnotifyAllIllegalMonitorStateException11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (con) {
                try {
                    con.notifyAll();
                    ConstructorExObjectnotifyAllIllegalMonitorStateException.res = ConstructorExObjectnotifyAllIllegalMonitorStateException.res - 40;
                } catch (IllegalMonitorStateException e2) {
                    ConstructorExObjectnotifyAllIllegalMonitorStateException.res = ConstructorExObjectnotifyAllIllegalMonitorStateException.res - 30;
                }
            }
        }
    }
}
@CustomAnnotations14(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass14 {
    private String sampleField;
    @CustomAnnotations14(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass14(String str) {
    }
    public String getSampleField() {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE, ElementType.METHOD, ElementType.CONSTRUCTOR})
@interface CustomAnnotations14 {
    String name();
    String value();
}

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
import java.lang.reflect.AccessibleObject;
public class AccessibleObjectGetDeclaredAnnotationsByTypeExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = accessibleObjectgDeclaredAnnotationsByTypeNullPointerException();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * Exception in AccessibleObject:public T[] getAnnotationsByType(Class<T> annotationClass).
     * @return status code
     * @throws NoSuchMethodException
     * @throws SecurityException
    */

    public static int accessibleObjectgDeclaredAnnotationsByTypeNullPointerException()
            throws NoSuchMethodException, SecurityException {
        int result1 = 4; /* STATUS_FAILED*/

        AccessibleObject mt = Test01a.class.getMethod("getName", new Class[] {});
        try {
            Object file1 = mt.getDeclaredAnnotationsByType(null);
            processResult -= 10;
        } catch (NullPointerException e1) {
            processResult -= 1;
        }
        return result1;
    }
}
class Test01a {
    private String name = "default";
    public String getName() {
        return name;
    }
}

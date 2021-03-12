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


import java.lang.reflect.Field;
import java.util.List;
class FieldToString<E, F> {
    public List<F> list1;
    private List<E> list2;
}
public class RTFieldToString {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldToString");
            Field instance1 = cls.getField("list1");
            Field instance2 = cls.getDeclaredField("list2");
            String q1 = instance1.toString();
            String q2 = instance2.toString();
            if (q1.equals("public java.util.List FieldToString.list1")
                    && q2.equals("private java.util.List FieldToString.list2")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
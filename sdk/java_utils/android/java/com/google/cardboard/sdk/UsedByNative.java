package com.google.cardboard.sdk;

import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

/**
 * Annotation used for marking methods and fields that are called from native code. Useful for
 * keeping components that would otherwise be removed by ProGuard.
 */
@Target({ElementType.METHOD, ElementType.FIELD, ElementType.TYPE, ElementType.CONSTRUCTOR})
public @interface UsedByNative {}

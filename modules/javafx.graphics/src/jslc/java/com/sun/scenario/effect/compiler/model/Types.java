package com.sun.scenario.effect.compiler.model;

public class Types {

    public static final Type VOID = new Type(BaseType.VOID, "void", 1);
    public static final Type FLOAT = new Type(BaseType.FLOAT, "float", 1);
    public static final Type FLOAT2 = new Type(BaseType.FLOAT, "float2", 2);
    public static final Type FLOAT3 = new Type(BaseType.FLOAT, "float3", 3);
    public static final Type FLOAT4 = new Type(BaseType.FLOAT, "float4", 4);
    public static final Type INT = new Type(BaseType.INT, "int", 1);
    public static final Type INT2 = new Type(BaseType.INT, "int2", 2);
    public static final Type INT3 = new Type(BaseType.INT, "int3", 3);
    public static final Type INT4 = new Type(BaseType.INT, "int4", 4);
    public static final Type BOOL = new Type(BaseType.BOOL, "bool", 1);
    public static final Type BOOL2 = new Type(BaseType.BOOL, "bool2", 2);
    public static final Type BOOL3 = new Type(BaseType.BOOL, "bool3", 3);
    public static final Type BOOL4 = new Type(BaseType.BOOL, "bool4", 4);
    public static final Type SAMPLER = new Type(BaseType.SAMPLER, "sampler", 1);
    public static final Type LSAMPLER = new Type(BaseType.SAMPLER, "lsampler", 1);
    public static final Type FSAMPLER = new Type(BaseType.SAMPLER, "fsampler", 1);
}

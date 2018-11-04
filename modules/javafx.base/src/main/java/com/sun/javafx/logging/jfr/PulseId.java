package com.sun.javafx.logging.jfr;

import static java.lang.annotation.ElementType.FIELD;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import jdk.jfr.Description;
import jdk.jfr.Name;
import jdk.jfr.Relational;

/**
 * This annotation defines a relation for future events that are related to the
 * same pulse id. It also informs a user interface consuming events where a
 * field contains a pulse id that the value could be useful to find other,
 * related events.
 */
@Relational
@Name("javafx.jfr.PulseId")
@Retention(RUNTIME)
@Target(FIELD)
@Description("Binds events with same pulse id together")
public @interface PulseId {

}

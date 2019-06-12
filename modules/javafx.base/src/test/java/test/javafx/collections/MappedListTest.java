package test.javafx.collections;

import javafx.beans.binding.Bindings;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.collections.transformation.MappedList;
import javafx.collections.transformation.SortedList;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import static org.junit.Assert.assertEquals;

public class MappedListTest {
    @Test
    public void testListCreation() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);
    }

    @Test
    public void testListAdd() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);

        observableList.add(0);

        assertEquals(List.of("3", "7", "1", "5", "0"), mappedList);
        assertEquals(List.of("3", "7", "1", "5", "0"), actual);
    }

    @Test
    public void testListRemove() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);

        observableList.remove(2);

        assertEquals(List.of("3", "7", "5"), mappedList);
        assertEquals(List.of("3", "7", "5"), actual);
    }

    @Test
    public void testListUpdate() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);

        observableList.set(2, 4);

        assertEquals(List.of("3", "7", "4", "5"), mappedList);
        assertEquals(List.of("3", "7", "4", "5"), actual);
    }

    @Test
    public void testListPermutation() {
        final SortedList<Integer> sortedList = FXCollections.observableArrayList(List.of(3, 7, 1, 5))
                .sorted(Comparator.naturalOrder());
        final MappedList<String, Integer> mappedList = new MappedList<>(sortedList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("1", "3", "5", "7"), mappedList);
        assertEquals(List.of("1", "3", "5", "7"), actual);

        sortedList.comparatorProperty().set(Comparator.comparing(String::valueOf).reversed());

        assertEquals(List.of("7", "5", "3", "1"), mappedList);
        assertEquals(List.of("7", "5", "3", "1"), actual);
    }

    @Test
    public void testMapperChange() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);

        mappedList.setMapper(i -> i + "!");

        assertEquals(List.of("3!", "7!", "1!", "5!"), mappedList);
        assertEquals(List.of("3!", "7!", "1!", "5!"), actual);
    }

    @Test
    public void testMapperChangeToNull() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList, String::valueOf);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);

        mappedList.setMapper(null);

        assertEquals(Collections.emptyList(), mappedList);
        assertEquals(Collections.emptyList(), actual);
    }

    @Test
    public void testMapperChangeFromNull() {
        final ObservableList<Integer> observableList = FXCollections.observableArrayList(List.of(3, 7, 1, 5));
        final MappedList<String, Integer> mappedList = new MappedList<>(observableList);

        final List<String> actual = new ArrayList<>();

        Bindings.bindContent(actual, mappedList);

        assertEquals(Collections.emptyList(), mappedList);
        assertEquals(Collections.emptyList(), actual);

        mappedList.setMapper(String::valueOf);

        assertEquals(List.of("3", "7", "1", "5"), mappedList);
        assertEquals(List.of("3", "7", "1", "5"), actual);
    }
}

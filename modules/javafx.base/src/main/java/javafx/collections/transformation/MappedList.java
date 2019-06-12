package javafx.collections.transformation;

import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.ListChangeListener;
import javafx.collections.ListChangeListener.Change;
import javafx.collections.ObservableList;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.IntStream;

/**
 * An implementation of a mapped {@link ObservableList}, which maps the values of the source {@link ObservableList} into
 * values of the target type {@link E}.
 *
 * @param <E> The instance type of the target elements
 * @param <F> The instance type of the source elements
 */
public class MappedList<E, F> extends TransformationList<E, F> {
    /**
     * The mapper function used to map the source values to the target type {@link E}.
     * If the mapper function is set to <code>null</code> the list acts as if it were empty
     */
    private final ObjectProperty<Function<? super F, ? extends E>> mapper;

    /**
     * A list of all mapped values
     */
    private final List<E> mappedValues;

    /**
     * Constructor
     *
     * @param source The source list
     * @param mapper The mapper function
     */
    public MappedList(ObservableList<? extends F> source, ObjectProperty<Function<? super F, ? extends E>> mapper) {
        super(source);

        this.mapper = mapper;
        this.mappedValues = new ArrayList<>();

        // create a cache of all mapped source elements
        Optional.ofNullable(getMapper())
                .ifPresent(mapperFunction -> source.stream().map(mapperFunction).forEach(mappedValues::add));

        // add a listener to detect changes of the mapper function
        mapper.addListener((observable, oldMapper, newMapper) -> {
            beginChange();
            // the previous mapper function was not null -> remove all values
            if (oldMapper != null) {
                final List<E> removed = new ArrayList<>(mappedValues);

                mappedValues.clear();

                nextRemove(0, removed);
            }

            // the current mapper function is not null -> calculate new values
            if (newMapper != null) {
                for (F element : getSource()) {
                    mappedValues.add(newMapper.apply(element));
                }

                nextAdd(0, size());
            }
            endChange();
        });

        // fire an initialisation event containing all mapped elements
        fireInitialisationChange();
    }

    /**
     * Constructor
     *
     * @param source The source list
     * @param mapper The mapper function
     */
    public MappedList(ObservableList<? extends F> source, Function<? super F, ? extends E> mapper) {
        this(source, new SimpleObjectProperty<>(mapper));
    }

    /**
     * Constructor
     *
     * @param source The source list
     */
    public MappedList(ObservableList<? extends F> source) {
        this(source, new SimpleObjectProperty<>());
    }

    /**
     * Fires a {@link Change} event containing all elements inside this {@link TransformationList}.
     * This method should be used directly after the {@link TransformationList} has been initialized
     */
    private void fireInitialisationChange() {
        beginChange();
        nextAdd(0, size());
        endChange();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sourceChanged(ListChangeListener.Change<? extends F> change) {
        beginChange();
        while (change.next()) {
            if (change.wasPermutated()) {
                permute(change);
            } else if (change.wasUpdated()) {
                update(change);
            } else {
                addRemove(change);
            }
        }
        endChange();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getSourceIndex(int index) {
        if (index >= size()) {
            throw new IndexOutOfBoundsException();
        }

        return index;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getViewIndex(int index) {
        if (index >= size()) {
            throw new IndexOutOfBoundsException();
        }

        return index;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public E get(int index) {
        if (index >= size()) {
            throw new IndexOutOfBoundsException();
        }

        return mappedValues.get(index);
    }

    /**
     * {@inheritDoc}
     * <p>
     * If no mapper function is set, the size of this list is always <code>0</code>, otherwise it equals
     * the size of the source list
     */
    @Override
    public int size() {
        return Optional.ofNullable(getMapper())
                .map(mapper -> getSource().size()).orElse(0);
    }

    private void permute(Change<? extends F> change) {
        final int from = change.getFrom();
        final int to = change.getTo();

        if (to > from) {
            final List<E> clone = new ArrayList<>(mappedValues);
            final int[] perm = IntStream.range(0, size()).toArray();

            for (int i = from; i < to; ++i) {
                perm[i] = change.getPermutation(i);
                mappedValues.set(i, clone.get(change.getPermutation(i)));
            }

            nextPermutation(from, to, perm);
        }
    }

    private void update(Change<? extends F> change) {
        final int from = change.getFrom();
        final int to = change.getTo();

        final Function<? super F, ? extends E> mapper = getMapper();

        if (mapper != null) {
            for (int i = from; i < to; ++i) {
                mappedValues.set(i, mapper.apply(getSource().get(i)));

                nextUpdate(i);
            }
        }
    }

    private void addRemove(Change<? extends F> change) {
        final int from = change.getFrom();

        final Function<? super F, ? extends E> mapper = getMapper();

        if (mapper != null) {
            for (int index = from + change.getRemovedSize() - 1; index >= from; index--) {
                nextRemove(index, mappedValues.remove(index));
            }

            for (int index = from; index < from + change.getAddedSize(); index++) {
                mappedValues.add(index, mapper.apply(getSource().get(index)));

                nextAdd(index, index + 1);
            }
        }
    }

    public Function<? super F, ? extends E> getMapper() {
        return mapper.get();
    }

    public void setMapper(Function<? super F, ? extends E> mapper) {
        this.mapper.set(mapper);
    }

    public ObjectProperty<Function<? super F, ? extends E>> mapperProperty() {
        return mapper;
    }
}

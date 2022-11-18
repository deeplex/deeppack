=========
 Streams 
=========


Output Streams/Buffer
---------------------

::

    #include <dplx/dp/streams/output_buffer.hpp>
    namespace dplx::dp {}

.. namespace:: dplx::dp

.. class:: output_buffer

    The (soon to be) universal way encoders interact with output streams. It is
    an abstract base class managing the memory being written to by the encoder
    and forwards the content to the underlying stream via :expr:`do_grow` and 
    :expr:`do_bulk_write`.

    :texpr:`output_buffer` models a ``contiguous_range`` of bytes which can be
    directly written. If an encoder needs more space than currently available
    it may call :expr:`ensure_size` which asks the underlying stream for the
    next buffer (and fails with :expr:`errc::end_of_stream` if it runs out of
    space).


    **Type Aliases**

    .. type:: element_type = std::byte
    .. type:: value_type = std::byte
    .. type:: pointer = std::byte *
    .. type:: iterator = std::byte *
    .. type:: size_type = std::size_t
    .. type:: difference_type = std::ptrdiff_t


    **Special Member Functions**

    .. function:: protected ~output_buffer() noexcept = default

        The destructor is trivial but ``protected`` to prevent accidental
        deletion of the base instead of the derived object.

    .. function:: protected output_buffer() noexcept

        Initializes the :texpr:`output_buffer` to an empty range/buffer.

    .. function:: output_buffer(output_buffer const &) noexcept = default

        The copy constructor is trivial but ``protected`` to prevent accidental
        slicing. 

    .. function:: auto operator=(output_buffer const &) noexcept \
                        -> output_buffer & = default

        The copy assignment operator is trivial but ``protected`` to prevent 
        accidental slicing.


    .. function:: protected output_buffer(std::byte *buffer, \
                                       std::size_t bufferSize) noexcept

        Initializes the instance with the given buffer. This should be used to
        avoid a :expr:`do_grow` call on the first write if possible.


    **Member Functions**

    .. function:: void commit_written(size_type numBytes)

        Informs the buffer that :expr:`numBytes` have been written. The buffer
        will remove these bytes from the beginning of the ``output_range`` and
        therefore shrink by :expr:`numBytes`.

    .. function:: auto ensure_size(size_type requestedSize) noexcept \
                        -> result<void>    

        Tries to get a buffer from the underlying stream which is at least
        :expr:`requestedSize` bytes big. May invalidate all iterators and
        pointers.

        If the current buffer doesn't satisfy the new size requirement
        :expr:`do_grow` will be called which may fail if no more space is
        available.

        .. warning:: 
            
            Data written to the current buffer which has not been committed by 
            calling :expr:`commit_written()` is not guaranteed to survive and
            might be lost.

    .. function:: private virtual auto do_grow(size_type requestedSize) \
                        noexcept -> result<void> = 0

        Asks the derived class to :expr:`reset()` this instance with a buffer *at least* as big as :expr:`requestedSize`. The implementation is
        responsible for actually outputting the committed area of the current
        buffer (which _might_ be a no-op if streaming to memory).
    
    .. function:: protected void reset() noexcept

        Resets this output range to an empty state.

    .. function:: protected void reset(std::byte *buffer, \
                                       std::size_t bufferSize)

        Resets this output range to ``[buffer, buffer + bufferSize)``.

    .. function:: auto bulk_write(std::byte const *bytes, \
                                  std::size_t bytesSize) noexcept \
                        -> result<void>

        Fills the current buffer starting at :expr:`begin()` with the data
        pointed at by :expr:`bytes` and commits these bytes as if calling
        :expr:`commit_written()` afterwards. If more bytes are being written 
        than can fit into the current buffer, the remainder gets forwarded to 
        :expr:`do_bulk_write()`.

    .. function:: private auto do_bulk_write(std::byte const *bytes, \
                                             std::size_t bytesSize) noexcept \
                        -> result<void> \
                        = 0

        Asks the derived class to output the current buffer and append the given
        data to the stream. The current buffer is guaranteed to be full i.e.
        :expr:`empty()` returns true. The implementation is encouraged to 
        :expr:`reset()` this instance to a (not necessarily) new buffer to 
        avoid a :expr:`do_grow()` call.

    **Contiguous Output Range Functions**

    .. function:: [[nodiscard]] auto begin() noexcept -> iterator
    .. function:: [[nodiscard]] auto end() noexcept -> iterator
    .. function:: [[nodiscard]] auto data() noexcept -> pointer
    .. function:: [[nodiscard]] auto size() const noexcept -> size_type
    .. function:: [[nodiscard]] auto empty() const noexcept -> bool

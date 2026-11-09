API reference
=============

These declarations are read from the same source files as the packages and C++ facade. A documentation build
therefore reflects the checked-out revision rather than a separately maintained copy of the signatures.

For a runnable example, start with :doc:`getting-started`. The module guides explain implementation details.

JavaScript and TypeScript
-------------------------

Load the built package from ``build/wasm/dist/index.js``. The declarations below describe its public interface.

.. literalinclude:: ../wasm/js/index.d.ts
   :language: typescript
   :linenos:

C++ entry points
----------------

Deck.hpp
~~~~~~~~

.. literalinclude:: ../include/oor/api/Deck.hpp
   :language: cpp
   :linenos:

CardTables.hpp
~~~~~~~~~~~~~~

.. literalinclude:: ../include/oor/api/CardTables.hpp
   :language: cpp
   :linenos:

CardEdits.hpp
~~~~~~~~~~~~~

.. literalinclude:: ../include/oor/api/CardEdits.hpp
   :language: cpp
   :linenos:

Queries.hpp
~~~~~~~~~~~

.. literalinclude:: ../include/oor/api/Queries.hpp
   :language: cpp
   :linenos:

Results.hpp
~~~~~~~~~~~

.. literalinclude:: ../include/oor/api/Results.hpp
   :language: cpp
   :linenos:

Design.hpp
~~~~~~~~~~

.. literalinclude:: ../include/oor/api/Design.hpp
   :language: cpp
   :linenos:

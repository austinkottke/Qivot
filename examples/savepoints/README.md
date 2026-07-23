# Tutorial — nested transactions (a savepoint sandbox)

Open nested transaction "scopes", edit rows inside them, then roll a scope back
to undo just its edits — while outer scopes keep theirs. The first scope is a
real `BEGIN`; deeper ones are `SAVEPOINT`s, so transactions compose.

> **Run it**
> ```sh
> cd examples/savepoints
> qmake && make
> ./savepoints
> ```

---

## The idea

`QiTransaction` nests. Each one you open bumps the depth; the outermost issues
`BEGIN`, inner ones `SAVEPOINT qi_sp_N`. Rolling back an inner scope does
`ROLLBACK TO` that savepoint — undoing only what changed since it opened.

```cpp
QiTransaction outer;          // BEGIN
a.save();                     // written
{
    QiTransaction inner;      // SAVEPOINT
    b.save();                 // written
    inner.rollback();         // ROLLBACK TO savepoint -> b is gone, a remains
}
outer.commit();               // COMMIT -> a is kept
```

## In the app

The controller keeps a stack of `QiTransaction*`:

```cpp
void SandboxStore::beginScope()    { m_stack.append(new QiTransaction()); }  // BEGIN / SAVEPOINT
void SandboxStore::rollbackScope() { auto *t = m_stack.takeLast(); t->rollback(); delete t; refresh(); }
void SandboxStore::commitScope()   { auto *t = m_stack.takeLast(); t->commit();   delete t; delete t...; }
```

Adding an item writes a real row *inside the current scope*, so it appears
immediately. A rollback isn't a write (it doesn't fire a change notification), so
the controller refreshes the list explicitly afterward — and the rolled-back rows
vanish.

**Try this:** Begin scope → add "A" → Begin scope → add "B" → **Roll back** (B
disappears, A stays, depth drops from `SP2` to `TX`) → **Commit** (A is kept).
That's exactly what the headless self-test asserts:

```text
in nested scopes, count: 3  depth: 2
after inner rollback, count: 2  depth: 1
after outer commit,  count: 2  (base was 1)
```

The depth strip at the top shows the open scopes — `TX` for the outer
transaction, `SP2`, `SP3`… for nested savepoints.

## Files

| File | Role |
|---|---|
| `models.h` | A one-field `Item` model. |
| `sandboxstore.h` / `.cpp` | The scope stack; add/remove + begin/rollback/commit. |
| `main.cpp` | In-memory DB with one committed row; loads the QML. |
| `main.qml` | The depth strip, scope controls, add row, and the item list. |

## See also

- [`schema`](../schema) — the single-level transaction and other schema features.

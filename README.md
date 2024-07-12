# Unikraft Cloud Scale-To-Zero integration extension

This extension integrates PostgreSQL into Unikraft Cloud's scale-to-zero mechanism.
To build and install the extension, ensure you have the development headers of PostgreSQL installed and then run:
```
make
sudo make install
```

Then add the following configuration to your `postgresql.conf`
```
shared_preload_libraries = 'pg_ukc_scaletozero'
```

Alternatively, you can also pass `-c shared_preload_libraries='pg_ukc_scaletozero'` to your `postgres` invocation.


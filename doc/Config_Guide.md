LDP Configuration Guide
=======================

##### Contents  
1\. Scheduling full updates  
Reference


1\. Scheduling full updates
---------------------------

Once per day, the LDP server runs a _full update_ which performs all
supported data updates.

Full updates can be scheduled at a preferred, recurring time of day by
setting `next_full_update` in table `ldpconfig.general`.  Note that
the timezone is part of the value.  For example:

```sql
UPDATE ldpconfig.general
    SET next_full_update = '2020-05-07 22:00:00Z';
```

This schedules the next full update on May 7, 2020 at 10:00 p.m. UTC.

Also ensure that `full_update_enabled` is set to `TRUE`.

A full update may take several hours for a large library.  During this
time, the database generally remains available to users, but query
performance may be affected.  Also, some stages of the update process
involve schema changes and could interrupt any long-running queries
that are executing at the same time.  For all of these reasons, it is
best to run full updates at a time when the database will not be used
heavily.


Reference
---------

### Table ldpconfig.general

* `full_update_enabled` (`BOOLEAN`) turns on daily full updates when
  set to `TRUE`.  No full updates are performed if it is set to
`FALSE`.

* `next_full_update` (`TIMESTAMP WITH TIME ZONE`) is the date and time
  of the next full update.  Once the full update begins, this value is
automatically incremented to the next day at the same time.

* `log_referential_analysis` (`BOOLEAN`) is reserved for future use.

* `force_referential_constraints` (`BOOLEAN`) is reserved for future
  use.


Further reading
---------------

[__Learn about installing and administering LDP in the Administrator Guide > > >__](Admin_Guide.md)

[__Learn about using the LDP database in the User Guide > > >__](User_Guide.md)



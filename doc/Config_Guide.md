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
that are executing at the same time.  For these reasons, it is best to
run full updates at a time when the database will be least heavily
used.


Reference
---------

### Table: ldpconfig.general

* `enable_full_updates` (BOOLEAN) turns on daily full updates when set
  to `TRUE`.  No full updates are performed if it is set to `FALSE`.
* `next_full_update` (TIMESTAMP WITH TIME ZONE) is the date and time
  of the next full update.  Once the full update begins, this value is
automatically incremented to the next day at the same time.
* `detect_foreign_keys` (BOOLEAN) enables experimental features that
  are not yet supported.
* `force_foreign_key_constraints` (BOOLEAN) enables experimental
  features that are not yet supported.
* `enable_foreign_key_warnings` (BOOLEAN) enables experimental
  features that are not yet supported.
<!--
* `detect_foreign_keys` (BOOLEAN) when set to `TRUE`, enables an
  experimental feature that attempts to detect or infer the presence
of foreign key relationships.  This process is run after a full
update.  The detected foreign keys are placed into the table
`ldpconfig.foreign_keys`.
* `force_foreign_key_constraints` (BOOLEAN) when set to `TRUE`,
  enables an experimental feature that applies foreign key constraints
listed in the table `ldpconfig.foreign_keys` where `enable_constraint`
is `TRUE`.  The constraints are applied after a full update.  In order
to define the constraints, rows that do not conform to them are
deleted.  Therefore this is a potentially destructive process and
should be used with care in a production system.  Note that history
tables generally retain all rows including those deleted by this
process.
* `enable_foreign_key_warnings` (BOOLEAN) is not currently supported.
-->
* `disable_anonymization` (BOOLEAN) when set to `TRUE`, disables
  anonymization of personal data.  Please read the section on "Data
privacy" in the [Administrator Guide](Admin_Guide.md) before changing
this setting.  As a safety precaution, the configuration setting
`disableAnonymization` in `ldpconf.json` also must be set by the LDP
system administrator.


Further reading
---------------

[__Learn about installing and administering LDP in the Administrator Guide > > >__](Admin_Guide.md)

[__Learn about using the LDP database in the User Guide > > >__](User_Guide.md)



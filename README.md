Library Data Platform
=====================

Copyright (C) 2019 The Open Library Foundation.  This software is distributed
under the terms of the Apache License, Version 2.0.  See the file
[LICENSE](https://github.com/folio-org/ldp/blob/master/LICENSE) for more
information.

**The Library Data Platform (LDP) is an open source platform for
reporting and analytics in libraries.  It extracts data from
[Okapi](https://github.com/folio-org/okapi)-based microservices and
produces a database that supports ad hoc, cross-domain queries and
integration with external data.**

```
id            | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
user_id       | ab579dc3-219b-4f5b-8068-ab1c7a55c402
proxy_user_id | a13dda6b-51ce-4543-be9c-eff894c0c2d0
item_id       | 459afaba-5b39-468d-9072-eb1685e0ddf4
loan_date     | 2017-02-03 08:43:15+00
due_date      | 2017-02-17 08:43:15+00
return_date   | 2017-03-01 11:35:12+00
action        | checkedin
item_status   | Available
renewal_count | 0
data          | {                                                          
              |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
              |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
              |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
              |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
              |     "loanDate": "2017-02-03T08:43:15Z",                    
              |     "dueDate": "2017-02-17T08:43:15.000+0000",             
              |     "returnDate": "2017-03-01T11:35:12Z",                  
              |     "action": "checkedin",                                 
              |     "itemStatus": "Available",                             
              |     "renewalCount": 0,                                     
              |     "status": {                                            
              |         "name": "Closed"                                   
              |     },                                                     
              | }
```

[**Learn about using the LDP database in the User Guide > > >**](USER_GUIDE.md)

[**Learn about installing and administering the LDP in the Admin Guide > > >**](ADMIN_GUIDE.md)

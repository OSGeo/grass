# Test Data

Test data was downloaded from the City of Raleigh, NC open data portal.
The data is a subset of the Daily Police Incidents dataset filtered from 2022 - 2024.

[Daily_Police_Incidents](https://services.arcgis.com/v400IkDOw1ad7Yad/arcgis/rest/services/Daily_Police_Incidents/FeatureServer/0/query?where=1%3D1&outFields=*&outSR=3358&f=json)

```bash
v.import input=https://services.arcgis.com/v400IkDOw1ad7Yad/arcgis/rest/ \
services/Daily_Police_Incidents/FeatureServer/0/query?where=1%3D1 \
&outFields=*&outSR=3358&f=json layer=ESRIJSON output=raleigh_crime_2022_2024
```

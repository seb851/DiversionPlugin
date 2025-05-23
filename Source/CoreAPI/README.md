## Configure openapi generator
- Install java
- install generator
```shell
npm install @openapitools/openapi-generator-cli -g
```

## Generate agentapi

Agent exposed API, as defined in [coreapi.yml](./coreapi.yml)

```shell
openapi-generator-cli generate -i coreapi.yml -g cpp-ue4 -c config.yaml -o . -t ..\..\openapi_templates\
```

Run "generate visual studio project file"

## Configure openapi generator
- Install java
- install generator
```shell
npm install @openapitools/openapi-generator-cli -g
```

## Generate agentapi

Agent exposed API, as defined in [agentapi.yaml](./agentapi.yaml)

```shell
openapi-generator-cli generate -i agentapi.yaml -g cpp-ue4 -c config.yaml -o . -t ..\..\openapi_templates\
```

Run "generate visual studio project file"

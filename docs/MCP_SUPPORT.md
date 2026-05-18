# MCP Support

NST exposes a lightweight MCP client in `QtLingo` so LLM orchestration can be
implemented separately from translation providers.

Current scope:

- stdio MCP servers launched as child processes
- `initialize` lifecycle handshake
- `notifications/initialized`
- `tools/list`
- `tools/call`
- raw JSON-RPC request, response, notification, and error helpers/signals

The client intentionally does not decide when an LLM should call tools. A caller
can list MCP tools, pass those tool definitions to any LLM provider, call the
selected tool through `McpClient::callTool()`, and then feed the result back to
the model.

## Example

```cpp
#include <qtlingo/mcpclient.h>

auto *client = new qtlingo::McpClient(this);

qtlingo::McpServerConfig config;
config.name = "filesystem";
config.command = "npx";
config.arguments = {"-y", "@modelcontextprotocol/server-filesystem", "/path/to/project"};

connect(client, &qtlingo::McpClient::initialized, client, [client]() {
    client->listTools();
});

connect(client, &qtlingo::McpClient::toolsListed,
        this, [](const QList<qtlingo::McpTool> &tools, const QString &nextCursor) {
            Q_UNUSED(nextCursor);
            for (const auto &tool : tools) {
                qDebug() << tool.serverName << tool.name << tool.description;
            }
        });

client->start(config);
```

To call a tool:

```cpp
QJsonObject arguments;
arguments["path"] = "/path/to/project/README.md";
client->callTool("read_file", arguments);
```

Handle the result:

```cpp
connect(client, &qtlingo::McpClient::toolCallFinished,
        this, [](const QString &toolName, const QJsonObject &result) {
            qDebug() << "MCP tool finished:" << toolName << result;
        });
```

## Config Shape

`McpServerConfig::fromVariantMap()` accepts this shape:

```json
{
  "name": "filesystem",
  "command": "npx",
  "args": ["-y", "@modelcontextprotocol/server-filesystem", "/path/to/project"],
  "workingDirectory": "/path/to/project",
  "env": {
    "EXAMPLE_TOKEN": "..."
  },
  "enabled": true
}
```

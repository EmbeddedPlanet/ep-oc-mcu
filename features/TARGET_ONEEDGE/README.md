# Telit OneEdge Integration Service

## Enabling support for the OneEdge integration service
To enable support for the Telit OneEdge integration service, you need to add the `"ONEEDGE"` label to your `mbed_app.json` file:

```json
{
    "target_overrides": {
        "*": {
            "target.extra_labels_add"                   : ["ONEEDGE"]
        }
    }
}
```
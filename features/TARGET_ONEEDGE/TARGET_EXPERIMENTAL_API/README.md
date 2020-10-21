# EP OC MCU Experimental APIs

## Enabling support for the EP OC MCU experimental APIs
To enable support for the EP OC MCU experimental APIs, you need to add the `"EXPERIMENTAL_API"` label to your `mbed_app.json` file:

```json
{
    "target_overrides": {
        "*": {
            "target.extra_labels_add"                   : ["EXPERIMENTAL_API"]
        }
    }
}
```
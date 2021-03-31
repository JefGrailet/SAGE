#!/bin/bash
# pods.sh: checks the overall state of all existing pods. No arg required.

namespace=my-authority # TODO: change this !
kubectl -n $namespace get pods -o=custom-columns=NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName

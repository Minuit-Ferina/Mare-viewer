# Payment Buy Dialog Helpers Summary

Date: 2026-05-24

Branch: `phase14`

## Summary

This packet applies the dialog-local UI helper pattern to purchase and payment
floaters.

No transaction behavior changes are intended. The patch only moves existing
button setup, visibility, enabled-state, text sync, and amount-control access
behind owner methods.

## Files Changed

- `indra/newview/llfloateraddpaymentmethod.*`
- `indra/newview/llfloaterbuy.*`
- `indra/newview/llfloaterbuycontents.*`
- `indra/newview/llfloaterbuycurrency.cpp`
- `indra/newview/llfloaterpay.cpp`

## Ownership Changes

- `LLFloaterAddPaymentMethod` now owns button setup, payment notification
  dispatch, and continue URL lookup through local methods.
- `LLFloaterBuy` now owns buy dialog control setup, list reset, title sync, and
  purchase text sync through local methods.
- `LLFloaterBuyContents` now owns control setup, item-list reset, purchase text
  sync, buy-button state, and wear-option sync through local methods.
- `LLFloaterBuyCurrencyUI` now owns button setup, buy-button state,
  status-widget hiding, target amount sync, and balance summary sync through
  local methods.
- `LLFloaterPay` now owns quick-pay button setup, custom amount control
  visibility, pay-button state, amount reads, object-name text sync, and amount
  focus through local methods.

## Verification

Targeted object build passed for:

- `llfloateraddpaymentmethod.cpp.o`
- `llfloaterbuy.cpp.o`
- `llfloaterbuycontents.cpp.o`
- `llfloaterbuycurrency.cpp.o`
- `llfloaterpay.cpp.o`

Architecture checks passed:

- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory scanned 3085 source files.


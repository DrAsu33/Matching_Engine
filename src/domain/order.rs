use std::fmt;

/// Maximum order identifier accepted by the current direct-index engine implementation.
pub const MAX_ORDER_ID_EXCLUSIVE: u64 = 12_000_000;
/// Exclusive upper bound of the engine's integer price domain.
pub const MAX_PRICE_EXCLUSIVE: u64 = 100_000;

/// Order side with a stable one-byte representation across the Rust/C++ FFI boundary.
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Side {
    Bid = 0,
    Ask = 1,
}

/// Validated order DTO shared with the C++ engine through its C ABI.
///
/// Private fields ensure safe Rust callers construct orders through [`Order::new`].
#[repr(C)]
#[derive(Debug)]
pub struct Order {
    id: u64,
    user_id: u64,
    side: Side,
    price: u64,
    amount: u64,
}

/// Validation failure returned while constructing an [`Order`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrderValidationError {
    OrderIdOutOfRange,
    PriceOutOfRange,
    ZeroAmount,
}

impl fmt::Display for OrderValidationError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::OrderIdOutOfRange => write!(
                formatter,
                "Order ID must be less than {}.",
                MAX_ORDER_ID_EXCLUSIVE
            ),
            Self::PriceOutOfRange => write!(
                formatter,
                "Price must be between 1 and {}.",
                MAX_PRICE_EXCLUSIVE - 1
            ),
            Self::ZeroAmount => write!(formatter, "Amount must be greater than 0."),
        }
    }
}

impl std::error::Error for OrderValidationError {}

impl Order {
    pub fn new(
        id: u64,
        user_id: u64,
        side: Side,
        price: u64,
        amount: u64,
    ) -> Result<Self, OrderValidationError> {
        if id >= MAX_ORDER_ID_EXCLUSIVE {
            return Err(OrderValidationError::OrderIdOutOfRange);
        }
        if !(1..MAX_PRICE_EXCLUSIVE).contains(&price) {
            return Err(OrderValidationError::PriceOutOfRange);
        }
        if amount == 0 {
            return Err(OrderValidationError::ZeroAmount);
        }

        Ok(Self {
            id,
            user_id,
            side,
            price,
            amount,
        })
    }

    pub fn id(&self) -> u64 {
        self.id
    }

    pub fn user_id(&self) -> u64 {
        self.user_id
    }

    pub fn side(&self) -> Side {
        self.side
    }

    pub fn price(&self) -> u64 {
        self.price
    }

    pub fn amount(&self) -> u64 {
        self.amount
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn constructs_valid_order() {
        let order = Order::new(1, 1001, Side::Bid, 100, 25).unwrap();

        assert_eq!(order.id(), 1);
        assert_eq!(order.user_id(), 1001);
        assert_eq!(order.side(), Side::Bid);
        assert_eq!(order.price(), 100);
        assert_eq!(order.amount(), 25);
    }

    #[test]
    fn rejects_invalid_order_id() {
        let result = Order::new(MAX_ORDER_ID_EXCLUSIVE, 1001, Side::Bid, 100, 25);

        assert_eq!(result.unwrap_err(), OrderValidationError::OrderIdOutOfRange);
    }

    #[test]
    fn rejects_invalid_price() {
        let zero_price = Order::new(1, 1001, Side::Bid, 0, 25);
        let max_price = Order::new(1, 1001, Side::Bid, MAX_PRICE_EXCLUSIVE, 25);

        assert_eq!(
            zero_price.unwrap_err(),
            OrderValidationError::PriceOutOfRange
        );
        assert_eq!(
            max_price.unwrap_err(),
            OrderValidationError::PriceOutOfRange
        );
    }

    #[test]
    fn rejects_zero_amount() {
        let result = Order::new(1, 1001, Side::Bid, 100, 0);

        assert_eq!(result.unwrap_err(), OrderValidationError::ZeroAmount);
    }
}
